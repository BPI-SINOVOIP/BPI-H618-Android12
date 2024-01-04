package com.softwinner.dragonbox.utils;

import java.io.InputStream;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import com.softwinner.dragonbox.entity.NetReceivedResult;

import android.util.Log;


public class XmlUtil {
    public static final String TAG = "DragonBox-XmlUtil";
    public static void parseXML(InputStream inputStream,NetReceivedResult netReceivedResult) {
        try {
            DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            DocumentBuilder db = dbf.newDocumentBuilder();
            Document doc = db.parse(inputStream);
            Element soapEnvelope = doc.getDocumentElement();//获取根元素aosp
            Log.e(TAG,soapEnvelope.getNodeName());

            NodeList nListSoapBody = soapEnvelope.getChildNodes();
            int i;
            for(i=0;i<nListSoapBody.getLength();i++) {
                if(nListSoapBody.item(i).getNodeName().equals("soap:Body"))
                    break;
            }
            NodeList nListCommandResponse = nListSoapBody.item(i).getChildNodes();
            for(i=0;i<nListCommandResponse.getLength();i++) {
                if(nListCommandResponse.item(i).getNodeName().contains("Response"))
                    break;
            }
            NodeList nListCommandResult = nListCommandResponse.item(i).getChildNodes();
            for(i=0;i<nListCommandResult.getLength();i++) {
                if(nListCommandResult.item(i).getNodeName().contains("Result")) {
                    String sNodeName = nListCommandResult.item(i).getNodeName();
                    netReceivedResult.sCommand = sNodeName.replace("Result", "");
                    break;
                }
            }
            NodeList nListCommandResultItem = nListCommandResult.item(i).getChildNodes();
            Log.e(TAG,"length = "+nListCommandResultItem.getLength());
            for(i=0;i<nListCommandResultItem.getLength();i++) {
                if(nListCommandResultItem.item(i).getNodeName().contains("anyType")) {
                    if(netReceivedResult.sResult==null)
                        netReceivedResult.sResult = nListCommandResultItem.item(i).getTextContent();
                    else {
                        netReceivedResult.sReason = nListCommandResultItem.item(i).getTextContent();
                    }
                }

            }
            Log.e(TAG,netReceivedResult.toString());
        } catch (Exception e) {
            // TODO: handle exception
        }
    }
}
