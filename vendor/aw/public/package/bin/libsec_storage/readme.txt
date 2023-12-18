

/*
*
*            libsec_storage
*
*/

工作原理： 读写用户的私密数据（MAC、ID等）
        --- 读机制：先从secure storage里面读出私密数据，如果数据没被破坏，正常返回；
                                如果发现数据被破坏了，则从private分区里面读出私密数据。
                                
        --- 写机制：一份数据同时会保存到private分区和secure storage里面。
        
        
使用方法： 具体接口只有两个，读和写私密数据，详情请看api.h
        
        
        

