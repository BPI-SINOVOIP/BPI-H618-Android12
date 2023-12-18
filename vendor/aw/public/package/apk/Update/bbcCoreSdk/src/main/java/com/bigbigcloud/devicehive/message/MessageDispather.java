package com.bigbigcloud.devicehive.message;

import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Created by Administrator on 2016/3/26.
 */
public class MessageDispather {
    private final String TAG = MessageDispather.class.getName();

    private Map<String, List<CommandMessageListener>> commandMessageListeners = new HashMap<String, List<CommandMessageListener>>();

    private Map<String, List<NotificationMessageListener>> notificationMessageListeners = new HashMap<String, List<NotificationMessageListener>>();

    private List<ConnectionListener> conectionListeners = new LinkedList<ConnectionListener>();

    private static class SingletonHolder{
        public static final MessageDispather INSTANCE = new MessageDispather();
    }

    private MessageDispather(){}

    public static final MessageDispather getInstance(){
        return SingletonHolder.INSTANCE;
    }

    public interface CommandMessageListener{
        void onMessage(BbcMessage message);
    }

    public interface NotificationMessageListener{
        void onMessage(BbcMessage message);
    }

    public interface ServerMessageListener{
        void onMessage(BbcMessage message);
    }

    public void registerMessageListener(CommandMessageListener listener, List<String> messageTypes){
        for(String messageType : messageTypes){
            List<CommandMessageListener> listeners;
            if(commandMessageListeners.containsKey(messageType)){
                listeners = commandMessageListeners.get(messageType);
            }else {
                Log.d(TAG, " fisrt register type " + messageType);
                listeners = new ArrayList<CommandMessageListener>();
                commandMessageListeners.put(messageType, listeners);
            }
            listeners.add(listener);
        }
    }

    public void unRegisterMessageListener(CommandMessageListener listener, List<String> messageTypes){
        for (String messageType : messageTypes){
            List<CommandMessageListener> listeners = commandMessageListeners.get(messageType);
            if(listeners != null){
                listeners.remove(listener);
            }
        }
    }

    public void notifyCommandMessageReceive(BbcMessage message){
        if(commandMessageListeners.containsKey(message.getMessageType())) {
            List<CommandMessageListener> listeners = commandMessageListeners.get(message.getMessageType());
            for (CommandMessageListener listener : listeners) {
                listener.onMessage(message);
            }
        }
    }

    public void registerMessageListener(NotificationMessageListener listener, List<String> messageTypes){
        for(String messageType : messageTypes){
            List<NotificationMessageListener> listeners;
            if(notificationMessageListeners.containsKey(messageType)){
                listeners = notificationMessageListeners.get(messageType);
            }else {
                Log.d(TAG, " fisrt register type " + messageType);
                listeners = new ArrayList<NotificationMessageListener>();
                notificationMessageListeners.put(messageType, listeners);
            }
            listeners.add(listener);
        }
    }

    public void unRegisterMessageListener(NotificationMessageListener listener, List<String> messageTypes){
        for (String messageType : messageTypes){
            List<NotificationMessageListener> listeners = notificationMessageListeners.get(messageType);
            if(listeners != null){
                listeners.remove(listener);
            }
        }
    }

    public void notifyNotificationMessageReceive(BbcMessage message){
        if(notificationMessageListeners.containsKey(message.getMessageType())) {
            List<NotificationMessageListener> listeners = notificationMessageListeners.get(message.getMessageType());
            for (NotificationMessageListener listener : listeners) {
                listener.onMessage(message);
            }
        }
    }

    public void registerConnectionListener(ConnectionListener listener){
        conectionListeners.add(listener);
    }

    public void unRegisterConnectionListener(ConnectionListener listener){
        conectionListeners.remove(listener);
    }



    public void notifyConnectionInfo(ConnectionInfoType type){
        for (ConnectionListener listener : conectionListeners){
            listener.onConnectionInfo(type);
        }
    }

    public interface ConnectionListener {
        void onConnectionInfo(ConnectionInfoType type);
    }


    public static enum  ConnectionInfoType{
        CONECT_SUCESS,
        CONECT_FAILED,
        RECONNECT;
    }
}
