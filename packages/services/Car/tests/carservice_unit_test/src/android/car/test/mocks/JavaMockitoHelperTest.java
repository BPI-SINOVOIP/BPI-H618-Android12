/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.car.test.mocks;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Test;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public final class JavaMockitoHelperTest {

    private static final long TIMEOUT_MS = 1_000L;

    @Test
    public void testAwait_Semaphore() throws InterruptedException {
        Semaphore semaphore = new Semaphore(1);

        JavaMockitoHelper.await(semaphore, TIMEOUT_MS);

        assertThat(semaphore.availablePermits()).isEqualTo(0);
    }

    @Test
    public void testAwait_CountDownLatch() throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        new Thread(() -> latch.countDown(), "testAwait_CountDownLatch").start();

        JavaMockitoHelper.await(latch, TIMEOUT_MS);

        assertThat(latch.getCount()).isEqualTo(0);
    }

    @Test
    public void testSilentAwait_notCalled() {
        CountDownLatch latch = new CountDownLatch(1);

        assertThat(JavaMockitoHelper.silentAwait(latch, 5L)).isFalse();
        assertThat(latch.getCount()).isEqualTo(1);
    }

    @Test
    public void testSilentAwait_called() {
        CountDownLatch latch = new CountDownLatch(1);
        new Thread(() -> latch.countDown(), "testSilentAwait_called").start();

        assertThat(JavaMockitoHelper.silentAwait(latch, TIMEOUT_MS)).isTrue();
        assertThat(latch.getCount()).isEqualTo(0);
    }

    @Test
    public void testGetResult() throws InterruptedException, ExecutionException, TimeoutException {
        Future<String> future = mock(Future.class);
        when(future.get(anyLong(), any())).thenReturn("done");

        assertThat(JavaMockitoHelper.getResult(future)).isEqualTo("done");
    }

    @Test
    public void testGetResult_withCustomTimeout()
            throws InterruptedException, ExecutionException, TimeoutException {
        Future<String> future = mock(Future.class);
        when(future.get(anyLong(), any(TimeUnit.class))).thenReturn("done");

        assertThat(JavaMockitoHelper.getResult(future, TIMEOUT_MS)).isEqualTo("done");
        verify(future).get(TIMEOUT_MS, TimeUnit.MILLISECONDS);
    }
}
