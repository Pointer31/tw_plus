# tw_plus
Modification of tw+ server by teetime, for the game Teeworlds.  
It includes dm, tdm and ctf gamemodes and the grenade and laser instagib modes for them, as well as the iFreeze gamemode.  
tw+ server by Teetime can be found here: https://github.com/Teetime/teeworlds/tree/tw-plus  
This modification adds a new gametype (Hold The Flag), map-based features, some ddnetclient support, and some other miscellaneous silly and useful additions.  

## How to build
**Bam**  
It can be built like a normal 0.6.3 teeworlds server.  
You may need bam 0.4.0 which can be downloaded from: https://www.teeworlds.com/files/bam-0.4.0.zip  
Build bam using <code>make_unix.sh</code> or one of the other files included in the download.  
Using bam, use build target server_debug:  
<code>./bam-0.4.0/bam server_debug</code>  

**CMake**  
The server can be built using CMake:  
<code>mkdir build
cd build
cmake ..
make</code>  

## Features
Apart from the features of the standard server as well as the tw+ server by teetime, the server include the following:  

**Map**  
multiple flag stands per map (one stand is chosen randomly)  
teleports (player, flag, and projectiles, (todo: laser?))  
grenade fountains  
slow killing death / slow healing / slow armor zone  
No-flag zone  
boost zone  

**Weapons**  
ninja (toggle invincibility, crazy-weapons, slash speed, heart/armor bonusses on pickup)  
toggleable silly custom weapons (laser->plasmagun, grenade->(wip)bouncy grenade, shotgun->explosive shotgun; repeater)  
pistol autoshooting  
(wip) indirect killing by hooking tee into spikes

**Other**  
shutdown message  
slash chat commands edited/added, including /me, /w, and /help  
Spectator stays spectator after map change  
broadcast \n (copied from DDnet)  
nicer log printing  
more types of characters in names allowed, like in ddnet  
end of round stats message  
DDnet client support (whisper, HUD)  
Experimental 64 player support (requires ddnet client)  
new gametype: Hold the flag (HTF); teamless mode where you gain points by holding the flag  

**Included Maps**  
included extra maps to show the new features:  
dm_island (slowdeath zone)  
dm_tele (teleports)  
dm_gfountain (teleports, grenade fountain)  
dm_towerravine_tele (teleports, boost)  
ctf_tele (teleports)  
ctf_sixflags (multiple flag stands)  
ctf_quadflags (multiple flag stands)  
ctf_flagged (teleports, health/shield zone and no-flag zone)  
htf_1 (shield, no-flag zone, HTF)  
htf_dm1 (shield, heart, no-flag zone, HTF) (made by yuiluy)  
htf_towerravine_tele (teleports, boost, HTF)  