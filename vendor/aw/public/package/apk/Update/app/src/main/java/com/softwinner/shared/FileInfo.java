package com.softwinner.shared;

import java.io.Serializable;

public class FileInfo {
    public final static int TYPE_URI = 0;
    public final static int TYPE_FILE = 1;

    public static class FileUnit implements Serializable {
        private static final long serialVersionUID = 1L;

        private String path = "";
        private int type = TYPE_URI;

        public FileUnit(String path, int type) {
            this.path = path;
            this.type = type;
        }

        public void setPath(String path) {
            this.path = path;
        }

        public void setType(int type) {
            this.type = type;
        }

        public int getType() {
            return type;
        }

        public String getPath() {
            return path;
        }
    }
}
