/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.net.networkstack

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.content.pm.PackageManager.MATCH_SYSTEM_ONLY
import android.net.INetworkStackConnector
import android.os.IBinder
import androidx.test.platform.app.InstrumentationRegistry
import kotlin.test.fail

/**
 * A [NetworkStackClientBase] that binds to [com.android.server.TestNetworkStackService]
 */
class TestNetworkStackServiceClient private constructor() : NetworkStackClientBase() {
    companion object {
        private val context by lazy { InstrumentationRegistry.getInstrumentation().context }
        private val networkStackVersion by lazy {
            val component = getNetworkStackComponent()
            val info = context.packageManager.getPackageInfo(component.packageName, 0 /* flags */)
            info.longVersionCode
        }

        /**
         * Create a [TestNetworkStackServiceClient] and connect it to the NetworkStack.
         */
        @JvmStatic
        fun connect(): TestNetworkStackServiceClient {
            return TestNetworkStackServiceClient().apply { init() }
        }

        @JvmStatic
        fun isSupported(): Boolean {
            // TestNetworkStackService was introduced in NetworkStack version 301100000.
            // It is also available at HEAD in development branches, where the version code is
            // 300000000.
            return networkStackVersion == 300000000L || networkStackVersion >= 301100000L
        }

        private fun getNetworkStackComponent(): ComponentName {
            val connectorIntent = Intent(INetworkStackConnector::class.java.name)
            return connectorIntent.resolveSystemService(context.packageManager, MATCH_SYSTEM_ONLY)
                    ?: fail("TestNetworkStackService not found")
        }
    }

    private val serviceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName, service: IBinder) {
            onNetworkStackConnected(INetworkStackConnector.Stub.asInterface(service))
        }

        override fun onServiceDisconnected(name: ComponentName) = Unit
    }

    private fun init() {
        val bindIntent = Intent(INetworkStackConnector::class.java.name + ".Test")
        bindIntent.component = getNetworkStackComponent()
        context.bindService(bindIntent, serviceConnection, Context.BIND_AUTO_CREATE)
    }

    fun disconnect() {
        InstrumentationRegistry.getInstrumentation().context.unbindService(serviceConnection)
    }
}