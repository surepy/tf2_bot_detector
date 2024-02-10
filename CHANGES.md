what's different from normal tf2bd?

### In-Game
* onGameJoin event (``exec tf2bd/onGameJoin.cfg``)  
* print joining marked players to party chat with names (instead of all chat)  
![oncheaterjoin](https://user-images.githubusercontent.com/24486494/214843728-aa1048c5-5f11-40bd-9865-7a90376bce6b.png)
* print existing marked players on server on join  
![onjoin print](https://user-images.githubusercontent.com/24486494/214842253-a60d3d58-be67-484f-bc11-3d7f1072a85a.png)
* Custom All-chat warning messages!  
![custom warnings](https://user-images.githubusercontent.com/24486494/232056440-5793e7e6-70f9-47ef-a879-ef9accc975da.png)
* Neuter chat wrappers (5 characters long, might break // makes copy-pasting from chat less painful.)
* Ability to set your own votekick "cooldown" (frequency of how often the tool tries to call a vote)
* Ability to ignore votekick team state for some maps
   * currently ``vsh_`` and ``ze_``
   * should rotate all "marked cheater" players, I haven't really got to test it a lot so probably report it if issues exist   
![votekick changes](https://github.com/surepy/tf2_bot_detector/assets/24486494/1e1c7423-6d6d-42bb-bc03-65f68bc62c92)

### Marking
* ability to mark people that left/disconnected on chat log.  
![image](https://user-images.githubusercontent.com/24486494/215266501-c1171fad-a848-49ec-862a-8c5acfa13f07.png)
* show reasons for mark on tooltip, if applicable.  
![mark reasons tooltip](https://user-images.githubusercontent.com/24486494/215059843-89f461bc-cebd-48c3-9e83-b24ababc463e.png)
* automatical marks from rules will come with "[auto] automatically marked" in reasons section, with chat if appliable  
![reasons for automatic mark](https://user-images.githubusercontent.com/24486494/215061544-6d00de40-514d-4b60-af3c-a9f86ce784c5.png)

### UI
* limited unicode support when using unifont font (new default, set in settings->UI if still question marks and u want unicode)  
currently cyrillic/chinese (simplified)/japanese/korean is added.  
![japanese (kanji) and korean](https://github.com/surepy/tf2_bot_detector/assets/24486494/21845fae-634e-4666-864a-2337e67436c1)
* "MarkedVS" to look at marked player count vs marked count (probably bots)  
format ``unmarked (unmarked_connected) vs marked (marked_connected)``   
  ![image](https://user-images.githubusercontent.com/24486494/224540394-b2612d24-30d4-4852-9e21-b90f78670cc4.png) 
* a working "reasons/proof" field (both on scoreboard and chat)  
![marking the user with reason](https://user-images.githubusercontent.com/24486494/216663458-589da5e6-9780-411b-8317-741b9c79e8b9.jpg)  
![verifying the reason](https://user-images.githubusercontent.com/24486494/216663482-7fa5ea6c-690d-4182-bd83-c2956fffd044.jpg)
* kill logs in chat _(default disabled)_  
![killogs](https://user-images.githubusercontent.com/24486494/232056583-ba99f610-423d-4096-879c-a4eb0cfea8ba.png)
 * +shows suicides too
* Tooltip now shows marked as name, date and generally more more concise
![new tooltip](https://github.com/surepy/tf2_bot_detector/assets/24486494/95fac417-43c2-45ca-a90f-793ec430c512)
* Added SteamHistory SourceBans integration  
![steamhistory integration](https://github.com/surepy/tf2_bot_detector/assets/24486494/c0ea2102-df0d-4767-a24f-fc6a0f57c23f)
 * first time setup now shows to setup the steam api (generate api key button opens on steam app)  
 ![first time setup](https://github.com/surepy/tf2_bot_detector/assets/24486494/38bab41a-24af-4f82-af45-968236e04adc) 
 * Added "Marked Friends" section (will not load if private)  
![marked friends](https://github.com/surepy/tf2_bot_detector/assets/24486494/1a85a00d-44db-448a-b6c4-9ab09e469f59)
* "Has SourceBans Entries" tooltip on playerlist  
![has sourcebans tooltip](https://github.com/surepy/tf2_bot_detector/assets/24486494/793fbf8b-cce9-4bf9-96f3-b6c3ab6682a0)

### Misc
* **64-bit TF2 support**
![image](https://github.com/surepy/tf2_bot_detector/assets/24486494/66933098-887f-457e-ac18-8428c68888fe)
  * legacy is ``hl2.exe``
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
* completely removed pazer's proxy service 
* **Fixed crash with huds that have chat wrappers already, were saved in utf8 (#6)**
     * budhud crashed cuz of this (credit: name)
* **Fix double loading of playerlist(s) (no more duplicate entries)**
* having a private profile won't cause you to have like [1gb log files by the end of your session](https://github.com/surepy/tf2_bot_detector/issues/17)
* static rcon launch parameter settings (not really recommended but if u _need_ them here u go)   
![image](https://github.com/surepy/tf2_bot_detector/assets/24486494/205c0fa4-59e5-4217-b2db-b337da32db54)
* bumped fmt to version 8.1.1 from 8.0.1 (but nobody cares)
* **Removed winrt (tf2_bot_detector_winrt)**
   * "why?"  
      It wasn't being used in the current codebase at all, like, at all at all.   
      It was only being used at ``tf2_bot_detector::Platform::Get(blahblah)Dir``, only when the application is in package mode; I don't have a package or "non-portable" build, and honestly just bloat cuz it's never used- so removing.   
      I can always just reintroduce it if I need it (I probably won't)
* **Removed imgui_desktop dependency and bumped imgui to 1.8 to 1.9**
  * moved logic into TF2BDApplication in the process, not "everything inside mainwindow class"
  * **removed support for anything other than opengl 4.3**
     *  this brings up the min requirements to geforce 400 series with _driver 310.61 (released 2012.11.20)_ and probably something similar in amd side.
     * "why?"  
       do you really not have a gtx 400 series in the year 2023? how are you even playing tf2?   
  * added renderer settings, you can now set maximum frame (vsync ~ 15fps) time of tf2bd. (this setting does not save)
![renderer settings](https://github.com/surepy/tf2_bot_detector/assets/24486494/e3c99aab-7f94-4b46-bde6-2772edc0bd50)  
     * default update rate max of tf2bd is now 60 (previously: vsync)
  * settings window is now handled by imgui using [multi-viewports feature](https://github.com/ocornut/imgui/wiki/Multi-Viewports) (+ it docks!)
  * window will now unsleep if you hover over the window.
* Ability to launch tf2 bot detector directly from steam (and have tf2 launch at the same time with auto-launch feature)

  https://github.com/surepy/tf2_bot_detector/assets/24486494/2e044925-a947-40e3-8302-fc2540e27b20
  * _"why?"_  
    to that I answer: why not?
  * _how do I use this?_  
   Set your launch option to ``<directory-to-bot-detector>tf2_bot_detector.exe %command% -forward "<your command line arguments>"``
  * **note that steam overlay may or may not freak the fuck out when using this feature, use it at your own risk lmfao**
* TF2BD will not infinitely retry rcon connections and leak ports on launch screen (#25)
  * lol.
* more todo...
