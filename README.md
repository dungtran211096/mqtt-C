# BTL_NetworkProgramming
Chat program bases on MQTT protocol 


## Function

- Create room chat : %channel_name
- Chat with a friend : $user_name
- Chat with many friends in a group chat 
- Take part in a group chat 
- Transfer file : #file_name
- User list : !list
			
## Usage 

Server 

```
$ git clone https://github.com/dungtran211096/mqtt-C.git
$ cd mqtt-C/Server 
$ make 
$ ./server
```

Client 

```
$ git clone https://github.com/dungtran211096/mqtt-C.git
$ cd mqtt-C/Client
$ make
$ ./client <IP_server>
```
