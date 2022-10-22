# BaseInjector
Base LoadLibraryA DLL Injector Source
![image](https://user-images.githubusercontent.com/80198020/197363435-0fcde0bb-e897-4ee5-b7bd-69f78da167bc.png)

## USAGE
- create a file named "engine_config.ini" and place it in the same directory as "BaseInjector.exe" (or use the pre made file in "CONFIG")  
- find the DLL you would like to inject and place this in the directory as well  
- open "engine_config.ini" tpye in the target process and dll name seperated by a ","  
- after following the instructions, add the launcher to your steam game library.  

Launch the injector before launching the target process. The injector will wait for the process to launch and then will immediately inject the selected dll.

Example config
`notepad.exe,XInput_4.dll`
