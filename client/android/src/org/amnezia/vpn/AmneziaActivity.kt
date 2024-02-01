package org.amnezia.vpn

import android.content.ComponentName
import android.content.Intent
import android.content.Intent.EXTRA_MIME_TYPES
import android.content.Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.net.Uri
import android.net.VpnService
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.Message
import android.os.Messenger
import android.webkit.MimeTypeMap
import android.widget.Toast
import androidx.annotation.MainThread
import androidx.core.content.ContextCompat
import java.io.IOException
import kotlin.LazyThreadSafetyMode.NONE
import kotlin.text.RegexOption.IGNORE_CASE
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.getStatistics
import org.amnezia.vpn.protocol.getStatus
import org.amnezia.vpn.qt.QtAndroidController
import org.amnezia.vpn.util.Log
import org.qtproject.qt.android.bindings.QtActivity

private const val TAG = "AmneziaActivity"

private const val CHECK_VPN_PERMISSION_ACTION_CODE = 1
private const val CREATE_FILE_ACTION_CODE = 2
private const val OPEN_FILE_ACTION_CODE = 3
private const val BIND_SERVICE_TIMEOUT = 1000L

class AmneziaActivity : QtActivity() {

    private lateinit var mainScope: CoroutineScope
    private val qtInitialized = CompletableDeferred<Unit>()
    private var isWaitingStatus = true
    private var isServiceConnected = false
    private var isInBoundState = false
    private lateinit var vpnServiceMessenger: IpcMessenger
    private var tmpFileContentToSave: String = ""

    private val vpnServiceEventHandler: Handler by lazy(NONE) {
        object : Handler(Looper.getMainLooper()) {
            override fun handleMessage(msg: Message) {
                val event = msg.extractIpcMessage<ServiceEvent>()
                Log.d(TAG, "Handle event: $event")
                when (event) {
                    ServiceEvent.CONNECTED -> {
                        QtAndroidController.onVpnConnected()
                    }

                    ServiceEvent.DISCONNECTED -> {
                        QtAndroidController.onVpnDisconnected()
                        doUnbindService()
                    }

                    ServiceEvent.RECONNECTING -> {
                        QtAndroidController.onVpnReconnecting()
                    }

                    ServiceEvent.STATUS -> {
                        if (isWaitingStatus) {
                            isWaitingStatus = false
                            msg.data?.getStatus()?.let { (state) ->
                                QtAndroidController.onStatus(state.ordinal)
                            }
                        }
                    }

                    ServiceEvent.STATISTICS_UPDATE -> {
                        msg.data?.getStatistics()?.let { (rxBytes, txBytes) ->
                            QtAndroidController.onStatisticsUpdate(rxBytes, txBytes)
                        }
                    }

                    ServiceEvent.ERROR -> {
                        msg.data?.getString(ERROR_MSG)?.let { error ->
                            Log.e(TAG, "From VpnService: $error")
                        }
                        // todo: add error reporting to Qt
                        QtAndroidController.onServiceError()
                    }
                }
            }
        }
    }

    private val activityMessenger: Messenger by lazy(NONE) {
        Messenger(vpnServiceEventHandler)
    }

    private val serviceConnection: ServiceConnection by lazy(NONE) {
        object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
                Log.d(TAG, "Service ${name?.flattenToString()} was connected")
                // get a messenger from the service to send actions to the service
                vpnServiceMessenger.set(Messenger(service))
                // send a messenger to the service to process service events
                vpnServiceMessenger.send {
                    Action.REGISTER_CLIENT.packToMessage().apply {
                        replyTo = activityMessenger
                    }
                }
                isServiceConnected = true
                if (isWaitingStatus) {
                    vpnServiceMessenger.send(Action.REQUEST_STATUS)
                }
            }

            override fun onServiceDisconnected(name: ComponentName?) {
                Log.w(TAG, "Service ${name?.flattenToString()} was unexpectedly disconnected")
                isServiceConnected = false
                vpnServiceMessenger.reset()
                isWaitingStatus = true
                QtAndroidController.onServiceDisconnected()
            }

            override fun onBindingDied(name: ComponentName?) {
                Log.w(TAG, "Binding to the ${name?.flattenToString()} unexpectedly died")
                doUnbindService()
                doBindService()
            }
        }
    }

    private data class CheckVpnPermissionCallbacks(val onSuccess: () -> Unit, val onFail: () -> Unit)

    private var checkVpnPermissionCallbacks: CheckVpnPermissionCallbacks? = null

    /**
     * Activity overloaded methods
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.d(TAG, "Create Amnezia activity: $intent")
        mainScope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
        vpnServiceMessenger = IpcMessenger(
            onDeadObjectException = ::doUnbindService,
            messengerName = "VpnService"
        )
        intent?.let(::processIntent)
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        Log.d(TAG, "onNewIntent: $intent")
        intent?.let(::processIntent)
    }

    private fun processIntent(intent: Intent) {
        // disable config import when starting activity from history
        if (intent.flags and FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY == 0) {
            if (intent.action == ACTION_IMPORT_CONFIG) {
                intent.getStringExtra(EXTRA_CONFIG)?.let {
                    mainScope.launch {
                        qtInitialized.await()
                        QtAndroidController.onConfigImported(it)
                    }
                }
            }
        }
    }

    override fun onStart() {
        super.onStart()
        Log.d(TAG, "Start Amnezia activity")
        mainScope.launch {
            qtInitialized.await()
            doBindService()
        }
    }

    override fun onStop() {
        Log.d(TAG, "Stop Amnezia activity")
        doUnbindService()
        super.onStop()
    }

    override fun onDestroy() {
        Log.d(TAG, "Destroy Amnezia activity")
        mainScope.cancel()
        super.onDestroy()
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            CREATE_FILE_ACTION_CODE -> {
                when (resultCode) {
                    RESULT_OK -> {
                        data?.data?.let { uri ->
                            alterDocument(uri)
                        }
                    }
                }
            }

            OPEN_FILE_ACTION_CODE -> {
                when (resultCode) {
                    RESULT_OK -> data?.data?.toString() ?: ""
                    else -> ""
                }.let { uri ->
                    QtAndroidController.onFileOpened(uri)
                }
            }

            CHECK_VPN_PERMISSION_ACTION_CODE -> {
                when (resultCode) {
                    RESULT_OK -> {
                        Log.d(TAG, "Vpn permission granted")
                        Toast.makeText(this, "Vpn permission granted", Toast.LENGTH_LONG).show()
                        checkVpnPermissionCallbacks?.run { onSuccess() }
                    }

                    else -> {
                        Log.w(TAG, "Vpn permission denied, resultCode: $resultCode")
                        Toast.makeText(this, "Vpn permission denied", Toast.LENGTH_LONG).show()
                        checkVpnPermissionCallbacks?.run { onFail() }
                    }
                }
                checkVpnPermissionCallbacks = null
            }

            else -> super.onActivityResult(requestCode, resultCode, data)
        }
    }

    /**
     * Methods for service binding
     */
    @MainThread
    private fun doBindService() {
        Log.d(TAG, "Bind service")
        Intent(this, AmneziaVpnService::class.java).also {
            bindService(it, serviceConnection, BIND_ABOVE_CLIENT)
        }
        isInBoundState = true
        handleBindTimeout()
    }

    @MainThread
    private fun doUnbindService() {
        if (isInBoundState) {
            Log.d(TAG, "Unbind service")
            isWaitingStatus = true
            QtAndroidController.onServiceDisconnected()
            vpnServiceMessenger.reset()
            isServiceConnected = false
            isInBoundState = false
            unbindService(serviceConnection)
        }
    }

    private fun handleBindTimeout() {
        mainScope.launch {
            if (isWaitingStatus) {
                delay(BIND_SERVICE_TIMEOUT)
                if (isWaitingStatus && !isServiceConnected) {
                    Log.d(TAG, "Bind timeout, reset connection status")
                    isWaitingStatus = false
                    QtAndroidController.onStatus(ProtocolState.DISCONNECTED.ordinal)
                }
            }
        }
    }

    /**
     * Methods of starting and stopping VpnService
     */
    private fun checkVpnPermissionAndStart(vpnConfig: String) {
        checkVpnPermission(
            onSuccess = { startVpn(vpnConfig) },
            onFail = QtAndroidController::onVpnPermissionRejected
        )
    }

    @MainThread
    private fun checkVpnPermission(onSuccess: () -> Unit, onFail: () -> Unit) {
        Log.d(TAG, "Check VPN permission")
        VpnService.prepare(applicationContext)?.let {
            checkVpnPermissionCallbacks = CheckVpnPermissionCallbacks(onSuccess, onFail)
            startActivityForResult(it, CHECK_VPN_PERMISSION_ACTION_CODE)
            return
        }
        onSuccess()
    }

    @MainThread
    private fun startVpn(vpnConfig: String) {
        if (isServiceConnected) {
            connectToVpn(vpnConfig)
        } else {
            isWaitingStatus = false
            startVpnService(vpnConfig)
            doBindService()
        }
    }

    private fun connectToVpn(vpnConfig: String) {
        Log.d(TAG, "Connect to VPN")
        vpnServiceMessenger.send {
            Action.CONNECT.packToMessage {
                putString(VPN_CONFIG, vpnConfig)
            }
        }
    }

    private fun startVpnService(vpnConfig: String) {
        Log.d(TAG, "Start VPN service")
        Intent(this, AmneziaVpnService::class.java).apply {
            putExtra(VPN_CONFIG, vpnConfig)
        }.also {
            ContextCompat.startForegroundService(this, it)
        }
    }

    private fun disconnectFromVpn() {
        Log.d(TAG, "Disconnect from VPN")
        vpnServiceMessenger.send(Action.DISCONNECT)
    }

    // saving file
    private fun alterDocument(uri: Uri) {
        try {
            contentResolver.openOutputStream(uri)?.use { os ->
                os.bufferedWriter().use { it.write(tmpFileContentToSave) }
            }
        } catch (e: IOException) {
            e.printStackTrace()
        }

        tmpFileContentToSave = ""
    }

    /**
     * Methods called by Qt
     */
    @Suppress("unused")
    fun qtAndroidControllerInitialized() {
        Log.v(TAG, "Qt Android controller initialized")
        qtInitialized.complete(Unit)
    }

    @Suppress("unused")
    fun start(vpnConfig: String) {
        Log.v(TAG, "Start VPN")
        mainScope.launch {
            checkVpnPermissionAndStart(vpnConfig)
        }
    }

    @Suppress("unused")
    fun stop() {
        Log.v(TAG, "Stop VPN")
        mainScope.launch {
            disconnectFromVpn()
        }
    }

    @Suppress("unused")
    fun saveFile(fileName: String, data: String) {
        Log.d(TAG, "Save file $fileName")
        mainScope.launch {
            tmpFileContentToSave = data

            Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "text/*"
                putExtra(Intent.EXTRA_TITLE, fileName)
            }.also {
                startActivityForResult(it, CREATE_FILE_ACTION_CODE)
            }
        }
    }

    @Suppress("unused")
    fun openFile(filter: String?) {
        Log.v(TAG, "Open file with filter: $filter")

        val mimeTypes = if (!filter.isNullOrEmpty()) {
            val extensionRegex = "\\*\\.[a-z .]+".toRegex(IGNORE_CASE)
            val mime = MimeTypeMap.getSingleton()
            extensionRegex.findAll(filter).map {
                mime.getMimeTypeFromExtension(it.value.drop(2))
            }.filterNotNull().toSet()
        } else emptySet()

        Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            Log.v(TAG, "File mimyType filter: $mimeTypes")
            when (mimeTypes.size) {
                1 -> type = mimeTypes.first()

                in 2..Int.MAX_VALUE -> {
                    type = "*/*"
                    putExtra(EXTRA_MIME_TYPES, mimeTypes.toTypedArray())
                }

                else -> type = "*/*"
            }
        }.also {
            startActivityForResult(it, OPEN_FILE_ACTION_CODE)
        }
    }

    @Suppress("unused")
    fun setNotificationText(title: String, message: String, timerSec: Int) {
        Log.v(TAG, "Set notification text")
    }

    @Suppress("unused")
    fun isCameraPresent(): Boolean = applicationContext.packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA)

    @Suppress("unused")
    fun startQrCodeReader() {
        Log.v(TAG, "Start camera")
        Intent(this, CameraActivity::class.java).also {
            startActivity(it)
        }
    }

    @Suppress("unused")
    fun setSaveLogs(enabled: Boolean) {
        Log.d(TAG, "Set save logs: $enabled")
        mainScope.launch {
            Log.saveLogs = enabled
            vpnServiceMessenger.send {
                Action.SET_SAVE_LOGS.packToMessage {
                    putBoolean(SAVE_LOGS, enabled)
                }
            }
        }
    }

    @Suppress("unused")
    fun exportLogsFile(fileName: String) {
        Log.v(TAG, "Export logs file")
        saveFile(fileName, Log.getLogs())
    }

    @Suppress("unused")
    fun clearLogs() {
        Log.v(TAG, "Clear logs")
        Log.clearLogs()
    }
}
