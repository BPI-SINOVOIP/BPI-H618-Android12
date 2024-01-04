package com.softwinner.agingdragonbox.engine;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

import com.softwinner.agingdragonbox.xml.Node;
import com.softwinner.agingdragonbox.xml.Parser;
import com.softwinner.agingdragonbox.xml.ParserException;
import com.softwinner.agingdragonbox.xml.parse.JaxpParser;

/**
 * 用户自定义添加apk测试case配置
 *
 * @author zengsc
 * @version date 2013-4-15
 */
public class CustomApkConfig {
    // 配置文件名
    public static final String FILENAME = "custom_activities.xml";

    public CustomApkConfig(String path) {
        File file = new File(path + FILENAME);
        InputStream is = null;
        if (file.exists()) {
            try {
                is = new FileInputStream(file);
                Parser parser = new JaxpParser();
                mNode = parser.parse(is);
            } catch (ParserException e) {
                e.printStackTrace();
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } finally {
                if (is != null) {
                    try {
                        is.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    private Node mNode;

    public int count() {
        if (mNode == null) {
            return 0;
        } else {
            return mNode.getNNodes();
        }
    }

    public Node getNode() {
        return mNode;
    }
}
