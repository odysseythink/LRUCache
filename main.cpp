#include <iostream>
#include <unistd.h>
#include "LRUCache.hh"

using namespace std;

int main(int argc, char *argv[])
{
    LRUCache<int, int> test_cache;
    test_cache.Init(10);
    test_cache.SetData(1, 1);
    int a = 0;
    if(test_cache.GetData(1, a))
    {
        cout<<"get ok:a="<<a<<endl;
    }
    else
    {
        cout<<"not find"<<endl;
    }

    test_cache.SetData(1, 19);

    if(test_cache.GetData(1, a))
    {
        cout<<"get ok:a="<<a<<endl;
    }
    else
    {
        cout<<"remove ok"<<endl;
    }    
 
    if(test_cache.Remove(1))
    {
        cout<<"rm ok"<<endl;
    }  
    
    if(test_cache.GetData(1, a))
    {
        cout<<"get ok:a="<<a<<endl;
    }
    else
    {
        cout<<"remove ok"<<endl;
    }
 
    test_cache.SetData(1, 1, 2);
    sleep(3);
    if(test_cache.GetData(1, a))
    {
        cout<<"get ok:a="<<a<<endl;
    }
    else
    {
        cout<<"expired ok"<<endl;
    }
 
    test_cache.SetData(1, 1, 3);
 
    test_cache.SetData(2, 2, 3);
    test_cache.GetData(2, a);
    test_cache.GetData(2, a);
    test_cache.GetData(2, a);
 
    test_cache.SetData(3, 3, 3);
    test_cache.SetData(4, 4, 3);
    test_cache.SetData(5, 5, 3);
    for(int i = 1; i < test_cache.Size(); i++)
    {
        if(test_cache.GetData(i, a))
            cout<<i<<":"<<a<<endl;
    }
    
    return 0;
}
