package com.softwinner.nfs;

/**
 * NFS Server shared folder wrapper
 * @author A
 *
 */
public class NFSFolder {
    private String folderPath = "";        // NFS server shared one folder path

    public String getFolderPath() {
        return folderPath;
    }

    public void setFolderPath(String folderPath1) {
        if (folderPath1 != null) {
            this.folderPath = folderPath1;
        }
    }
}
