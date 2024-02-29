package org.amnezia.vpn

import android.app.Application
import androidx.datastore.core.MultiProcessDataStoreFactory
import androidx.datastore.core.Serializer
import androidx.datastore.dataStoreFile
import java.io.InputStream
import java.io.ObjectInputStream
import java.io.ObjectOutputStream
import java.io.OutputStream
import java.io.Serializable
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.withContext
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.util.Log

private const val TAG = "VpnState"
private const val STORE_FILE_NAME = "vpnState"

data class VpnState(val protocolState: ProtocolState, val serverName: String? = null) : Serializable {
    companion object {
        val defaultState: VpnState = VpnState(DISCONNECTED)
    }
}

object VpnStateStore {
    private lateinit var app: Application

    private val dataStore = MultiProcessDataStoreFactory.create(
        serializer = VpnStateSerializer(),
        produceFile = { app.dataStoreFile(STORE_FILE_NAME) }
    )

    fun init(app: Application) {
        Log.v(TAG, "Init VpnStateStore")
        this.app = app
    }

    fun dataFlow(): Flow<VpnState> = dataStore.data

    suspend fun store(f: (vpnState: VpnState) -> VpnState) {
        dataStore.updateData(f)
    }
}

private class VpnStateSerializer : Serializer<VpnState> {
    override val defaultValue: VpnState = VpnState.defaultState

    override suspend fun readFrom(input: InputStream): VpnState {
        return withContext(Dispatchers.IO) {
            ObjectInputStream(input).use {
                it.readObject() as VpnState
            }
        }
    }

    override suspend fun writeTo(t: VpnState, output: OutputStream) {
        withContext(Dispatchers.IO) {
            ObjectOutputStream(output).use {
                it.writeObject(t)
            }
        }
    }
}
