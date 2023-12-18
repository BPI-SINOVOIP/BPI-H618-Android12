package com.softwinner.timerswitch;

import java.io.Serializable;
import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedList;

public class WeekdaysLinkList<E> extends LinkedList<E> implements Serializable {

    public WeekdaysLinkList() {
        super();
    }

    public WeekdaysLinkList(Collection<? extends E> c) {
        super(c);
    }

    @Override
    public String toString() {
        Iterator<E> it = iterator();
        StringBuilder sb = new StringBuilder();
        while (it.hasNext()){
            E e = it.next();
            sb.append(e.toString()).append(',');
        }
        String ret = sb.toString();
        if(ret.length() > 1 && ret.charAt(ret.length() - 1) == ','){
            ret = ret.substring(0, ret.length() - 1);
        }
        return ret;
    }
}
