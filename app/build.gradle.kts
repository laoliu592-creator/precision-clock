import java.util.Base64

plugins {
    id("com.android.application")
    kotlin("android")
}

android {
    namespace = "com.example.precisionclock"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.example.precisionclock"
        minSdk = 29
        targetSdk = 35
        versionCode = 1
        versionName = "1.0.0"

        ndk { abiFilters += listOf("arm64-v8a") }
        externalNativeBuild {
            cmake { cppFlags += listOf("-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wconversion", "-Wshadow", "-Wpedantic") }
        }
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlin { jvmToolchain(17) }

    signingConfigs {
        create("release") {
            val keystoreBase64 = System.getenv("RELEASE_KEYSTORE_BASE64")
            val storePasswordEnv = System.getenv("RELEASE_STORE_PASSWORD")
            val keyAliasEnv = System.getenv("RELEASE_KEY_ALIAS")
            val keyPasswordEnv = System.getenv("RELEASE_KEY_PASSWORD")

            if (!keystoreBase64.isNullOrBlank() &&
                !storePasswordEnv.isNullOrBlank() &&
                !keyAliasEnv.isNullOrBlank() &&
                !keyPasswordEnv.isNullOrBlank()
            ) {
                val keystoreFile = layout.buildDirectory.file(
                    "signing/precision-clock-release.jks"
                ).get().asFile
                keystoreFile.parentFile.mkdirs()
                keystoreFile.writeBytes(Base64.getDecoder().decode(keystoreBase64))
                storeFile = keystoreFile
                storePassword = storePasswordEnv
                keyAlias = keyAliasEnv
                keyPassword = keyPasswordEnv
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("release")
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
        debug { applicationIdSuffix = ".debug" }
    }

    externalNativeBuild { cmake { path = file("../CMakeLists.txt"); version = "3.30.5" } }
    packaging { jniLibs { useLegacyPackaging = false } }
    testOptions { unitTests.isIncludeAndroidResources = true }
}

dependencies {
    implementation("androidx.core:core-ktx:1.15.0")
    implementation("androidx.appcompat:appcompat:1.7.0")
    testImplementation("junit:junit:4.13.2")
}
