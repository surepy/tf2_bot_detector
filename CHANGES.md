what's different from normal tf2bd?

### In-Game
* onGameJoin event (``exec tf2bd/onGameJoin.cfg``)  
* print joining marked players to party chat with names (instead of all chat)  
![oncheaterjoin](https://user-images.githubusercontent.com/24486494/214843728-aa1048c5-5f11-40bd-9865-7a90376bce6b.png)
* print existing marked players on server on join  
![onjoin print](https://user-images.githubusercontent.com/24486494/214842253-a60d3d58-be67-484f-bc11-3d7f1072a85a.png)
* Custom All-chat warning messages!  
![custom warnings](https://user-images.githubusercontent.com/24486494/232056440-5793e7e6-70f9-47ef-a879-ef9accc975da.png)

### Marking
* ability to mark people that left/disconnected on chat log.  
![image](https://user-images.githubusercontent.com/24486494/215266501-c1171fad-a848-49ec-862a-8c5acfa13f07.png)
* show reasons for mark on tooltip, if applicable.  
![mark reasons tooltip](https://user-images.githubusercontent.com/24486494/215059843-89f461bc-cebd-48c3-9e83-b24ababc463e.png)
* automatical marks from rules will come with "[auto] automatically marked" in reasons section, with chat if appliable  
![reasons for automatic mark](https://user-images.githubusercontent.com/24486494/215061544-6d00de40-514d-4b60-af3c-a9f86ce784c5.png)

### UI
* "MarkedVS" to look at marked player count vs marked count (probably bots)  
format ``unmarked (unmarked_connected) vs marked (marked_connected)``
  ![image](https://user-images.githubusercontent.com/24486494/224540394-b2612d24-30d4-4852-9e21-b90f78670cc4.png) 
* a working "reasons/proof" field (both on scoreboard and chat)  
![marking the user with reason](https://user-images.githubusercontent.com/24486494/216663458-589da5e6-9780-411b-8317-741b9c79e8b9.jpg)  
![verifying the reason](https://user-images.githubusercontent.com/24486494/216663482-7fa5ea6c-690d-4182-bd83-c2956fffd044.jpg)
* kill logs in chat _(default disabled)_  
![killogs](https://user-images.githubusercontent.com/24486494/232056583-ba99f610-423d-4096-879c-a4eb0cfea8ba.png)

### Misc
* **Dropped support for all installation methods that aren't the portable one**  
    * Sorry, I really don't have time nor energy for that...
* Working version notifications for my fork (using github api)  
![updates](https://user-images.githubusercontent.com/24486494/227868425-a91405b6-2111-432d-a468-9c3151addc58.png)
* mastercomfig compatibility fix  
* chat logs (located at ``logs/chat``, disable in settings)
**NOTE: old chat logs will NOT BE DELETED AUTOMATCIALLY!**  
![image](https://user-images.githubusercontent.com/24486494/216662036-dca5a796-1a82-4ef6-95ad-33ec9622ea94.png)
* ability to disable console logging to file   
![console log disable](https://user-images.githubusercontent.com/24486494/216662532-88594df1-6fb7-4a99-bd02-73a7a13042fd.png)
* **75%+ performance gain | removed FaceIT client checks**  
    it won't warn you if you have it open anymore, please check manually before you go into a FaceIT server. 
* less painful building process  
    * don't have to set [CMAKE_TOOLCHAIN_FILE](https://github.com/surepy/tf2_bot_detector/commit/011ac8f4a656ff3406fa9a8ead268122cf0c2930) manually.
    * all the dependency version should be locked, so no stupid fmt breaking everything...
* more todo...
