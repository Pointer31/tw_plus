# tw_plus
Modification of tw+ server by teetime, for the game Teeworlds.  
It includes dm, tdm and ctf gamemodes and the grenade and laser instagib modes for them, as well as the iFreeze gamemode.  
tw+ server by Teetime can be found here: https://github.com/Teetime/teeworlds/tree/tw-plus  
This modification adds new gametypes (Hold The Flag (HTF), LMS, no-items DM), map-based features, some ddnetclient support, and some other miscellaneous silly and useful additions.  

## How to build
**Bam**  
It can be built like a normal 0.6.3 teeworlds server.  
You may need bam 0.4.0 which can be downloaded from: https://www.teeworlds.com/files/bam-0.4.0.zip  
Build bam using <code>make_unix.sh</code> or one of the other files included in the download.  
Using bam, use the following command:  
<code>./bam-0.4.0/bam</code>  

**CMake**  
The server can be built using CMake:  
<code>mkdir build
cd build
cmake ..
make</code>  

## Features
Apart from the features of the standard server as well as the tw+ server by teetime, the server include the following:  
Note that the folder ./client-files contains some data that may be useful when making maps for this mod. You may want to see the wiki for examples on how to add teleports and boost/jump pads.  

**Map**  
multiple flag stands per map (one stand is chosen randomly)  
teleports (player, flag, and projectiles, (todo: laser?))  
grenade fountains  
slow killing death / slow healing / slow armor zone  
No-flag zone  
boost zone, boost-pad  
custom weapon spawns  

**Weapons**  
ninja (toggle invincibility, crazy-weapons, slash speed, heart/armor bonusses on pickup)  
toggleable silly custom weapons (laser->plasmagun, grenade->(wip)bouncy grenade, shotgun->explosive shotgun; repeater)  
pistol autoshooting  
indirect killing by hooking tee into spikes  
custom weapons, of which spawns can be placed in a map: plasmagun, charge hammer, super pistol  

**Other**  
sv_touch_explode  
sv_instagib  
add_vote_if (check for certain gamemodes before adding vote)  
shutdown message  
slash chat commands edited/added, including /me, /w, and /help  
Spectator stays spectator after map change  
broadcast \n (copied from DDnet)  
nicer log printing  
more types of characters in names allowed, like in ddnet  
end of round stats message  
allow giving rcon password as commandline argument via --rconpwd  
DDnet client support (whisper, HUD)  
Experimental 64 player support (requires ddnet client)  
Particles appearing when a pickup (shield, heart, weapon) is respawning  
Particles for players with a killing spree (ddnet client 18.7+)  
Play a sound different from the normal hit sound when the player kills a tee, like in some other gametypes  
new gametype: Hold the flag (HTF); (teamless) mode where you gain points by holding the flag  
new gametype: Last man standing (LMS); (teamless) mode where you must avoid dying while killing others  
new gametype: No Items Death Match (nDM): teamless mode where you can only use one weapon, which is randomly chosen every 15s by default.  
Simple bots, with a few difficulty options (Thanks to +KZ for their skin, chosen as default skin for bots)  

**Included Maps**  
included extra maps to show the new features:  
dm_island (slowdeath zone)  
dm_tele (teleports)  
dm_gfountain (teleports, grenade fountain)  
dm_towerravine_tele (teleports, boost)  
dm_deserttower (boost-pad)  
ctf_tele (teleports)  
ctf_sixflags (multiple flag stands)  
ctf_quadflags (multiple flag stands)  
ctf_flagged (teleports, health/shield zone and no-flag zone)  
htf_1 (shield, no-flag zone, HTF)  
htf_dm1 (shield, heart, no-flag zone, HTF) (made by yuiluy)  
htf_towerravine_tele (teleports, boost, HTF)  
htf_skytower (boost-pad, HTF)  
htf_plush (boost-pad, teleports, HTF)  