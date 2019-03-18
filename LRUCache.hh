/**
 * @file      LRUCache.hh
 * @brief     
 * @details   
 * @author    ranwei
 * @version     
 * @date      2019/3/18 9:4:4:398
 * @copyright Unistrong
 * @par         (c) COPYRIGHT 2010-2018 by Unistrong Systems, Inc.    
 *                        All rights reserved.
 *                                                                    
 *       This software is confidential and proprietary to Unistrong 
 *     Systems, Inc.  No part of this software may be reproduced,    
 *     stored, transmitted, disclosed or used in any form or by any means
 *     other than as expressly provided by the written license agreement    
 *     between Unistrong Systems and its licensee.
 * @par History      
 *         1.Date         -- 2019/3/18 9:4:4:398
 *           Author       -- ranwei
 *           Modification -- Created file
 *
 */
#ifndef __LRUCACHE_HH__
#define __LRUCACHE_HH__

#ifdef  LRUCACHE_GLOBAL
#define LRUCACHE_EXT
#else
#define LRUCACHE_EXT extern
#endif /* LRUCACHE_GLOBAL */

/*============================================================================*/
/*                                  @INCLUDES                                 */
/*============================================================================*/
#include <stdlib.h>
#include <iostream>
#include <map>
#include <time.h>
#include <pthread.h>
#include <error.h>

using namespace std;

 
/** @addtogroup LRUCACHE
  * @{
  */
 
/*============================================================================*/
/*                             @MACROS & @TYPEDEFS                            */
/*============================================================================*/
#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)


                                                                                
/*============================================================================*/
/*                             @GLOBAL VIRIABLES                              */
/*============================================================================*/
                                                                                
/*============================================================================*/
/*                                   @FUNCS                                   */
/*============================================================================*/
                                                                                
/*============================================================================*/
/*                                   @CLASS                                   */
/*============================================================================*/
/*! @class LRUCache
 *  @brief LRU缓冲区 @anchor LRUCache_Details
 *   
 */ 	
template<class T1, class T2> 
class LRUCache
{ 
public: 
    struct ST_CacheData
    {
        time_t begin_time;
        time_t update_time;
        int expire_time;
        int count;
        T2 data;
    };
    LRUCache()
    {
        m_ThreadExitMutex = PTHREAD_MUTEX_INITIALIZER;
        m_Mutex = PTHREAD_MUTEX_INITIALIZER;
        m_ExpireThreadExit = false;
        m_MaxSize = 0;
        m_AvgExpireTime = 0;
    }
    ~LRUCache()
    {
        m_MaxSize = 0;      
        SetExpireThreadExit();
    }

    bool Init(int max_cache_size = 50)
    {
        int res;
        m_MaxSize = max_cache_size;
        pthread_t id;
        pthread_attr_t attr;
        int stack_size = -1;

        /* Initialize thread creation attributes */
        res = pthread_attr_init(&attr);
        if(res != 0){
            errno = res; 
            perror("pthread_attr_init");             
            return false;        
        }

        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);        
        if(res != 0){
            errno = res; 
            perror("pthread_attr_setdetachstate");             
            return false;        
        }        
        if(stack_size > 0){
            res = pthread_attr_setstacksize(&attr, stack_size);
            if(res != 0){
                errno = res; 
                perror("pthread_attr_setstacksize");  
                return false;
            }
        }
        cout << "init LRUCache 111" << endl;
        res = pthread_create(&m_ExpireThreadId, &attr, &ExpireThreadProc, this);
        if(res != 0) {
            errno = res; 
            perror("pthread_create");  
            return false;            
        }

        res = pthread_attr_destroy(&attr);
        if(res != 0){
            errno = res; 
            perror("pthread_attr_destroy"); 
            SetExpireThreadExit();
            return false;
        }

        return true;
    }

    /**
      * @brief      获取key对应的数据。
      * @anchor     LRUCache.Get
      * @details    根据key在cache中查找对应的数据。
      * @param[in]  key: 要查找的key。
      * @param[out] data: 存储查找到的数据。
      * @return     查找是否成功。
      * @retval     true: cache中找到key对应的记录。
      * @retval     false: cache中没有key对应的记录。
      * @note       
      * @par   History      :
      *           1.Date    -- 2019/3/18 14:47:3:356
      *             Author  -- ranwei
      *             Details -- Created function
      */
    bool GetData(const T1& key, T2& data)
    {
        pthread_mutex_lock(&m_Mutex);
        typename map<T1, ST_CacheData>::iterator it = m_CacheMap.find(key);
        if(it == m_CacheMap.end())
        {
            pthread_mutex_unlock(&m_Mutex);
            return false;
        }

        data = it->second.data;
        it->second.count++;
        it->second.update_time = time(NULL); // 获取当前时间
        pthread_mutex_unlock(&m_Mutex);
        return true;
    }

    /**
      * @brief      设置cache数据。
      * @anchor     LRUCache.SetData
      * @details    设置cache数据。如果key存在，则替换他的数据，如果不存在这新建节点存放数据。
      * @param[in]  key: 
      * @param[in]  data:
      * @param[in]  expire_time:
      * @return     
      * @retval     
      * @note       
      * @par   History      :
      *           1.Date    -- 2019/3/18 15:11:30:582
      *             Author  -- ranwei
      *             Details -- Created function
      */
    bool SetData(const T1& key, const T2& data, const int& expire_time = -1)
    {
        pthread_mutex_lock(&m_Mutex);
        typename map<T1, ST_CacheData>::iterator it = m_CacheMap.find(key);
        if(it == m_CacheMap.end()) { // cache中没有存这个key对应的记录
            ST_CacheData new_data;
            new_data.count = 0;
            new_data.expire_time = expire_time;
            new_data.begin_time = time(NULL);
            new_data.update_time = new_data.begin_time;
            new_data.data = data;
            if(m_CacheMap.size() >= m_MaxSize){
                pthread_mutex_unlock(&m_Mutex);
                __LRURemove();
                pthread_mutex_lock(&m_Mutex);
            }
            m_CacheMap.insert(pair<T1, ST_CacheData>(key, new_data));

        }else { // cache中存在这个key对应的记录，更新条记录的数据
            it->second.data = data;
            it->second.count++;
            it->second.update_time = time(NULL); // 获取当前时间   
            it->second.expire_time = expire_time;
        }
        pthread_mutex_unlock(&m_Mutex);
        return true;
    }

    /**
      * @brief      返回cache记录条数。
      * @anchor     LRUCache.Size
      * @details    
      * @return     返回cache的记录条数。
      * @retval     
      * @note       
      * @par   History      :
      *           1.Date    -- 2019/3/18 16:48:14:111
      *             Author  -- ranwei
      *             Details -- Created function
      */
    int Size()
    {
        return m_CacheMap.size();
    }

    /**
      * @brief      从cache中移除key对应的记录。
      * @anchor     LRUCache.Remove
      * @details    
      * @param[in]  key: 根据key，移除对应的记录。
      * @return     成功返回true，失败返回false。
      * @note       
      * @par   History      :
      *           1.Date    -- 2019/3/18 16:22:4:724
      *             Author  -- ranwei
      *             Details -- Created function
      */
    bool Remove(const T1& key)
    {
        pthread_mutex_lock(&m_Mutex);
        if(m_CacheMap.find(key) != m_CacheMap.end())
        {
            m_CacheMap.erase(key);
            pthread_mutex_unlock(&m_Mutex);
            return true;
        }

        pthread_mutex_unlock(&m_Mutex);
        return false;
    }

    int LRUExpire()
    {
        double timeflies;
        int count = 0;
        time_t now;
        pthread_mutex_lock(&m_Mutex);
        long long sum = 0;
        for(typename map<T1, ST_CacheData>::iterator it = m_CacheMap.begin();
            it != m_CacheMap.end();)
        {
            now = time(NULL);
            timeflies = difftime(now, it->second.update_time);
            sum += it->second.expire_time;
            if(it->second.expire_time >= 0
               && timeflies >= it->second.expire_time)
            {
                m_CacheMap.erase(it++);
                ++count;
            }
            else
            {
                ++it;
            }
        }

        if(!m_CacheMap.empty())
        {
            m_AvgExpireTime = sum / m_CacheMap.size();
        }
        pthread_mutex_unlock(&m_Mutex);

        return count;
    }

    int GetAvgExpireTime()
    {
        pthread_mutex_lock(&m_Mutex);
        int avg_expire_time = m_AvgExpireTime;
        pthread_mutex_unlock(&m_Mutex);
        return avg_expire_time;
    }
    bool CheckExpireThreadExit()
    {
        bool stat;
        pthread_mutex_lock(&m_ThreadExitMutex);
        stat = m_ExpireThreadExit;
        pthread_mutex_unlock(&m_ThreadExitMutex);
        return stat;
    }
    bool SetExpireThreadExit()
    {
        bool stat;
        pthread_mutex_lock(&m_ThreadExitMutex);
        m_ExpireThreadExit = true;
        pthread_mutex_unlock(&m_ThreadExitMutex);
        return stat;
    }    

private:
    bool __LRURemove()
    {
        double timeflies;        
        pthread_mutex_lock(&m_Mutex);
        typename map<T1, ST_CacheData>::iterator lru_it = m_CacheMap.begin();
        typename map<T1, ST_CacheData>::iterator it = m_CacheMap.begin();
        if(it == m_CacheMap.end())
        {
            pthread_mutex_unlock(&m_Mutex);
            return false;
        }
    
        time_t now = time(NULL);
        double maxtimeflies =  difftime(now, it->second.update_time);
        for(++it; it != m_CacheMap.end(); ++it)
        {
            now = time(NULL);
            timeflies =  difftime(now, it->second.update_time);
            
            if(maxtimeflies < timeflies)//lru
            {
                maxtimeflies = timeflies;
                lru_it = it;
            }
        }
        m_CacheMap.erase(lru_it);
        pthread_mutex_unlock(&m_Mutex);
        return true;
    }

    static void* ExpireThreadProc(void *args)
    {
        LRUCache *pInstance = (LRUCache *)args;
        while(!pInstance->CheckExpireThreadExit())
        {
            sleep(abs(pInstance->GetAvgExpireTime()/2));
            pInstance->LRUExpire();
        }
        return NULL;
    }    

private:
    int m_MaxSize;
    int m_AvgExpireTime;
    map<T1, ST_CacheData> m_CacheMap;
    pthread_mutex_t m_Mutex;
    pthread_t m_ExpireThreadId;
    pthread_mutex_t m_ThreadExitMutex;
    bool m_ExpireThreadExit;
};

                                                                               

/**
  * @}
  */ 
		
#endif /* __LRUCACHE_HH__ */
/*************** (C) COPYRIGHT 2010-2018 Unistrong ******END OF FILE***********/
