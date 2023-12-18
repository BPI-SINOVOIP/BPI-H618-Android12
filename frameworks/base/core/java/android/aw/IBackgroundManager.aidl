package android.aw;

import android.content.Intent;
import android.content.pm.ResolveInfo;

/**
 * System private interface to the background manager.
 *
 * {@hide}
 */
interface IBackgroundManager
{
    /**
     * set debug
     */
    void setDebug(boolean debug);

    /**
     * return user whitelist
     */
    List<String> getUserWhitelist();

    /**
     * set user whitelist
     * @param whitelist user whitelist
     */
    void setUserWhitelist(in List<String> whitelist);

    /**
     * return system whitelist
     */
    List<String> getSystemWhitelist();

    /**
     * set limit background count
     * @param limit background count
     */
    void setLimitBackgroundCount(int limit);

    /**
     * return limit background count
     */
    int getLimitBackgroundCount();

    /**
     * check skip service
     * @param Intent service intent
     * @param callingPackage calling package name
     */
    boolean skipService(in Intent service, String callingPackage);

    /**
     * resolver receivers and remove not in whitelist
     * @param Intent target intent
     * @param receivers resolver receivers
     */
    String[] resolverReceiver(in Intent intent, inout List<ResolveInfo> receivers);
}
