package com.example.playreadydemo;


import android.media.MediaDrm;
import android.media.NotProvisionedException;
import android.media.DeniedByServerException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.HttpClient;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.HttpResponse;
import org.apache.http.util.EntityUtils;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.Arrays;
import android.os.AsyncTask;
import android.util.Log;


public class KeyRequester {
    private final String TAG = "KeyRequester";

    private byte[] mInitData;
	private String mServerUrl;

    public KeyRequester(byte[] initData) {
        mInitData = initData;
    }

    public boolean doTransact(MediaDrm drm, byte[] sessionId) {

        boolean retryTransaction;
        boolean result = false;

        do {
            retryTransaction = false;

            MediaDrm.KeyRequest drmRequest = null;
            boolean retryRequest;
            do {
                retryRequest = false;

                try {
					String initStr = new String(mInitData);
					Log.d(TAG, "gqy**mInitData, size:" + mInitData.length + "str:" + initStr );
                    drmRequest = drm.getKeyRequest(sessionId, mInitData, "playready",
                                                   MediaDrm.KEY_TYPE_STREAMING, null);
                } catch (NotProvisionedException e) {
                    Log.i(TAG, "playready no need provision,but open session failed!!");
                    retryRequest = true;
                }
            } while (retryRequest);

            if (drmRequest == null) {
                Log.e(TAG, "Failed to get key request");
                return false;
            }
			mServerUrl = drmRequest.getDefaultUrl();
			Log.d(TAG, "serverUlr=" + mServerUrl);
            PostRequestTask postTask = new PostRequestTask(drmRequest.getData());
            postTask.execute(mServerUrl);

            // wait for post task to complete
            byte[] responseBody;
            long startTime = System.currentTimeMillis();

            do {
                responseBody = postTask.getResponseBody();
                if (responseBody == null) {
                    sleep(100);
                } else {
                    break;
                }
            } while (System.currentTimeMillis() - startTime < 5000);

            if (responseBody == null) {
                Log.e(TAG, "No response from license server!");
            } else {
                byte[] drmResponse = parseResponseBody(responseBody);
                if (drmResponse == null) {
                    Log.e(TAG, "No response body in response");
                } else {
                    try {
						String str = new String(drmResponse);
						Log.d(TAG, "gqy*****drmResponse, size: " + drmResponse.length);
					    Log.d(TAG, "drmResponse=" + str);
                        drm.provideKeyResponse(sessionId, drmResponse);
                        result = true;
                    } catch (NotProvisionedException e) {
                        Log.i(TAG, "Key response invalidated the certificate, reprovisioning");
                        retryTransaction = true;
                    } catch (DeniedByServerException e) {
                        // informational, the event handler will take care of provisioning
                        Log.e(TAG, "Server rejected the key request");
                    }
                }
            }
        } while (retryTransaction);
        return result;
    }

    private class PostRequestTask extends AsyncTask<String, Void, Void> {
        private final String TAG = "PostRequestTask";

        private byte[] mDrmRequest;
        private byte[] mResponseBody;

        public PostRequestTask(byte[] drmRequest) {
            mDrmRequest = drmRequest;
        }

        protected Void doInBackground(String... urls) {
            mResponseBody = postRequest(urls[0], mDrmRequest);
            Log.d(TAG, "response length=" + mResponseBody.length);
            return null;
        }

        public byte[] getResponseBody() {
            return mResponseBody;
        }

        private byte[] postRequest(String url, byte[] drmRequest) {
            Log.d(TAG, "PostRequest url=" + url);
            HttpClient httpclient = new DefaultHttpClient();
            HttpPost httppost = new HttpPost(url);

            try {
                // Add data
                ByteArrayEntity entity = new ByteArrayEntity(drmRequest);
				String hexrequst;
				hexrequst = toHexString(drmRequest);
				String requststring = new String(drmRequest);
				Log.d(TAG,"request string size="+drmRequest.length);
				Log.d(TAG, "requestring="+ requststring);
                httppost.setEntity(entity);
                httppost.setHeader("User-Agent", "MSPR_PK_UTILITY");
				httppost.setHeader("Pragma", "no-cache");
				httppost.setHeader("Content-Type", "text/xml; charset=utf-8");
				httppost.setHeader("SOAPAction", "\"http://schemas.microsoft.com/DRM/2007/03/protocols/AcquireLicense\"");

                Log.d(TAG, "request line=" + httppost.getRequestLine().toString());

                // Execute HTTP Post Request
                HttpResponse response = httpclient.execute(httppost);

                byte[] responseBody;
                int responseCode = response.getStatusLine().getStatusCode();
                if (responseCode == 200) {
                    responseBody = EntityUtils.toByteArray(response.getEntity());
                } else {
                    Log.d(TAG, "Server returned HTTP error code " + responseCode);
                    return null;
                }
                return responseBody;

            } catch (ClientProtocolException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
            return null;
        }
    }


    // validate the response body and return the drmResponse blob
    private byte[] parseResponseBody(byte[] responseBody) {
        String bodyString = null;
        try {
            bodyString = new String(responseBody, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

        if (bodyString == null) {
            return null;
        }

        if (bodyString.startsWith("GLS/")) {
            if (!bodyString.startsWith("GLS/1.")) {
                Log.e(TAG, "Invalid server version, expected 1.x");
                return null;
            }
            int drmMessageOffset = bodyString.indexOf("\r\n\r\n");
            if (drmMessageOffset == -1) {
                Log.e(TAG, "Invalid server response, could not locate drm message");
                return null;
            }
            responseBody = Arrays.copyOfRange(responseBody, drmMessageOffset + 4, responseBody.length);
        }
        return responseBody;
    }

    private void sleep(int msec) {
        try {
            Thread.sleep(msec);
        } catch (InterruptedException e) {
        }
    }
	private String toHexString(byte[] byteArray) {
  		if (byteArray == null || byteArray.length < 1)
   		throw new IllegalArgumentException("this byteArray must not be null or empty");

  		final StringBuilder hexString = new StringBuilder();
  		for (int i = 0; i < byteArray.length; i++) {
   		if ((byteArray[i] & 0xff) < 0x10)
    		hexString.append("0");
   			hexString.append(Integer.toHexString(0xFF & byteArray[i]));
  		}
  		return hexString.toString().toLowerCase();
 	}

}

