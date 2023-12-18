package com.bigbigcloud.devicehive.service;

import android.util.Log;
import android.util.Pair;

import com.bigbigcloud.devicehive.utils.Utils;
import com.bigbigcloud.devicehive.websocket.WebSocket;
import com.bigbigcloud.devicehive.websocket.WebSocketConnection;
import com.bigbigcloud.devicehive.websocket.WebSocketConnectionHandler;
import com.bigbigcloud.devicehive.websocket.WebSocketException;
import com.bigbigcloud.devicehive.websocket.WebSocketOptions;
import com.google.gson.JsonObject;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/3/24.
 */
public class WebsocketAgent extends RestAgent  {

    private static final String TAG = "WebsocketAgent";

    public static final String WS_AUTH_DEVICE_GUID_HEADER = "WS-Auth-DeviceGuid";
    public static final String WS_AUTH_DEVICE_SIGNATURE_HEADER = "WS-Auth-Signature";
    public static final String WS_AUTH_DEVICE_TIMESTAMP_HEADER = "WS-Auth-Timestamp";

    private final WebSocketConnection mConnection = new WebSocketConnection();
    private WebsocketCallback websocketCallback;

    private final String role;
    private final String websocketUri;

    private WebSocketConnectionHandler mWebsocketHandler = new WebSocketConnectionHandler(){
        @Override
        public void onOpen() {
            websocketCallback.onOpen();
        }

        @Override
        public void onClose(int code, String reason) {
            if(code == WebSocket.ConnectionHandler.CLOSE_RECONNECT){
                websocketCallback.onReconnect();
            }else {
                websocketCallback.onClose();
            }
        }

        @Override
        public void onTextMessage(String payload) {
            Log.d(TAG, " onTextMessage " + payload);
            websocketCallback.handleMessage(payload);
        }
    };

    public WebsocketAgent(String restUri,String websocketUri, String role){
        super(restUri);
        this.websocketUri = websocketUri;
        this.role = role;
    }

    public void connect(){
        try{
            WebSocketOptions options = new WebSocketOptions();
            options.setReconnectInterval(10000);
            long timestamp = System.currentTimeMillis()/1000;
            String strToEncode = principal.getPrincipal().first + timestamp + principal.getPrincipal().second;
            String signature = Utils.SHA1(strToEncode);
            List<Pair> headers = new ArrayList<>();
            headers.add(Pair.create(WS_AUTH_DEVICE_GUID_HEADER, principal.getPrincipal().first));
            headers.add(Pair.create(WS_AUTH_DEVICE_TIMESTAMP_HEADER, timestamp));
            headers.add(Pair.create(WS_AUTH_DEVICE_SIGNATURE_HEADER, signature));
            mConnection.connect(websocketUri + "/" + role, null, mWebsocketHandler, options, headers);
        } catch (WebSocketException e){
            Log.e(TAG, e.toString());
        }
    }

    public void disconnect(){
        mConnection.disconnect();
    }

    public boolean isConnected(){
        return mConnection.isConnected();
    }

    public void  rawSendMessage(final JsonObject message){
        mConnection.sendTextMessage(message.toString());
    }


    public void setWebsocketCallback(WebsocketCallback callback){
        this.websocketCallback = callback;
    }


    public interface WebsocketCallback{
        void onOpen();
        void onReconnect();
        void onClose();
        void handleMessage(String message);
    }

}
