plugins {
    id(libs.plugins.android.library.get().pluginId)
    id(libs.plugins.kotlin.android.get().pluginId)
}

kotlin {
    jvmToolchain(17)
}

android {
    namespace = "org.amnezia.vpn.util"

    buildFeatures {
        // add BuildConfig class
        buildConfig = true
    }
}

dependencies {
    implementation(libs.androidx.security.crypto)
}
