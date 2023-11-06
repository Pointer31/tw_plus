# tw_plus
Modification of tw+ server by teetime, for the game Teeworlds.  
It includes dm, tdm and ctf gamemodes and the grenade and laser instagib modes for them, as well as the iFreeze gamemode.  
tw+ server by teetime can be found here: https://github.com/Teetime/teeworlds/tree/tw-plus
## How to build
It can be built like a normal 0.6 teeworlds server.  
Note that you may need bam 0.4.0 which can be downloaded from: https://www.teeworlds.com/files/bam-0.4.0.zip  
Using bam, use build target server_debug.
## Features
Apart from the features of the standard server as well as the tw+ server by teetime, the server include the following:  
\
Simple and naive anti-adbot; It looks for discord invites in the first two messages sent by a player.  
Multiple flags support; when multiple flags are detected, the flag will spawn on one of the stands, randomly chosen, each time the flags reset.  
Plasmagun; Optionally, replacing the laser rifle, A gun that shoot two curved lasers.  
Crazy Ninja; Optionally, the ninja uses all other weapons at the same time as it slashes.  
Invincible Ninja, Optionally, the ninja is invincible.  
Auto pistol; Standard enabled, optionally the pistol can autofire.  
/me usage; Standard enabled, optionally players can use the /me command.  
