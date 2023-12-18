package com.softwinner.agingdragonbox.engine;

import com.softwinner.agingdragonbox.xml.Node;

public class NoneConfiguration extends BaseConfiguration {
    @Override
    protected void onInitializeForConfig(Node attr) {
        int resId = Utils.CASE_MAP.get(mCase.getClass().getSimpleName());
        setName(resId);
    }
}
