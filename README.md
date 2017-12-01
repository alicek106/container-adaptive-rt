# Saumsung RT

Adaptive RT Scheduling Agent for Docker Container  
Written by alicek106, 2017. 12. 01
    
<br>

# 1. How to use config.txt

> [Interval time to check process priority]  
[Container name] [scheduling policy] [priority] [process name] [enable lwp check]  
.... more information for process!

* **Interval time to check process priority** : Millisecond time to check processes.  
* **Container Name** : Your container name  
* **Scheduling Policy** : Select one scheduling policy from followings : *SCHED_OTHER, SCHED_FIFO, SCHED_RR*  
* **Priority** :
 * Case of SCHED_OTHER : -20 ~ 19  
 * Case of SCHED_FIFO and SCHED_RR : 1~99  

* **Process Name** : Your process name  
* **Enable lwp Check** : Whether check lwp process (In general, ps -ec just display normal processes, not lwp)  

<br>

# 2. Example of config.txt
In below case, we will change priority of process for only lwp processes.  

    1000
    rt-test SCHED_FIFO 80 ./cyclictest 1
----
In below case, we will change priority of process for all processes corresponding to regex.  

    1000
    rt-test SCHED_OTHER -10 apache2 0
    
<br>

# 3. How to run
    # git clone http://gitlab.icnslab.net/alicek106/samsung-adaptiveRT.git
    # cd samsung-adaptiveRT
    
    # docker run -d --cap-add=sys_nice --name rt-agent \
    --privileged --pid=host -e DOCKER_URI=192.168.0.44:2375 \
    -v "$PWD"/config.txt:/config.txt alicek106/rt-agent:0.2 ./AdaptiveRT