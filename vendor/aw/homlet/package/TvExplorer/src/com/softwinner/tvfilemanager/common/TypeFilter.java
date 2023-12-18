package com.softwinner.tvfilemanager.common;

public final class TypeFilter {
    public static boolean isMusicFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("mp3") || ext.equalsIgnoreCase("ogg")
                    || ext.equalsIgnoreCase("wav")
                    || ext.equalsIgnoreCase("wma")
                    || ext.equalsIgnoreCase("m4a")
                    || ext.equalsIgnoreCase("ape")
                    || ext.equalsIgnoreCase("dts")
                    || ext.equalsIgnoreCase("flac")
                    || ext.equalsIgnoreCase("mp1")
                    || ext.equalsIgnoreCase("mp2")
                    || ext.equalsIgnoreCase("aac")
                    || ext.equalsIgnoreCase("midi")
                    || ext.equalsIgnoreCase("mid")
                    || ext.equalsIgnoreCase("mp5")
                    || ext.equalsIgnoreCase("mpga")
                    || ext.equalsIgnoreCase("mpa")
                    || ext.equalsIgnoreCase("m4p")
                    || ext.equalsIgnoreCase("amr")
                    || ext.equalsIgnoreCase("m4r")
                    || ext.equalsIgnoreCase("AC3")
                    || ext.equalsIgnoreCase("M4R")
                    || ext.equalsIgnoreCase("RA")
                    || ext.equalsIgnoreCase("WMA")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }

        return false;
    }

    public static boolean isMovieFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("avi") || ext.equalsIgnoreCase("wmv")
                    || ext.equalsIgnoreCase("rmvb")
                    || ext.equalsIgnoreCase("mkv")
                    || ext.equalsIgnoreCase("m4v")
                    || ext.equalsIgnoreCase("m1v")
                    || ext.equalsIgnoreCase("mov")
                    || ext.equalsIgnoreCase("mpg")
                    || ext.equalsIgnoreCase("rm")
                    || ext.equalsIgnoreCase("flv")
                    || ext.equalsIgnoreCase("pmp")
                    || ext.equalsIgnoreCase("vob")
                    || ext.equalsIgnoreCase("dat")
                    || ext.equalsIgnoreCase("asf")
                    || ext.equalsIgnoreCase("psr")
                    || ext.equalsIgnoreCase("3gp")
                    || ext.equalsIgnoreCase("mpeg")
                    || ext.equalsIgnoreCase("ram")
                    || ext.equalsIgnoreCase("divx")
                    || ext.equalsIgnoreCase("m4p")
                    || ext.equalsIgnoreCase("m4b")
                    || ext.equalsIgnoreCase("mp4")
                    || ext.equalsIgnoreCase("f4v")
                    || ext.equalsIgnoreCase("3gpp")
                    || ext.equalsIgnoreCase("3g2")
                    || ext.equalsIgnoreCase("3gpp2")
                    || ext.equalsIgnoreCase("webm")
                    || ext.equalsIgnoreCase("ts") || ext.equalsIgnoreCase("tp")
                    || ext.equalsIgnoreCase("m2ts")
                    || ext.equalsIgnoreCase("3dv")
                    || ext.equalsIgnoreCase("3dm")
                    || ext.equalsIgnoreCase("ogm")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }

        return false;
    }

    public static boolean isPictureFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("png") || ext.equalsIgnoreCase("jpeg")
                    || ext.equalsIgnoreCase("jpg")
                    || ext.equalsIgnoreCase("gif")
                    || ext.equalsIgnoreCase("bmp")
                    || ext.equalsIgnoreCase("jfif")
                    || ext.equalsIgnoreCase("tiff")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }

        return false;
    }

    public static boolean isTxtFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("txt")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

    public static boolean isPdfFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("pdf")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

    public static boolean isWordFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("doc") || ext.equalsIgnoreCase("docx")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

    public static boolean isExcelFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("xls") || ext.equalsIgnoreCase("xlsx")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

    public static boolean isPptFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("ppt") || ext.equalsIgnoreCase("pptx")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

    public static boolean isHtml32File(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("html")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

    public static boolean isApkFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("apk")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

    public static boolean isISOFile(String path) {
        try {
            String ext = path.substring(path.lastIndexOf(".") + 1);
            if (ext.equalsIgnoreCase("iso")) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

    public static boolean isOtherFile(String path) {
        try {
            if (!(isMovieFile(path) && !isMusicFile(path) && !isPictureFile(path))) {
                return true;
            }
        } catch (IndexOutOfBoundsException e) {
            return false;
        }
        return false;
    }

}