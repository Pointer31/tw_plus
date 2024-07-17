# tw_plus
Modification of tw+ server by teetime, for the game Teeworlds.  
It includes dm, tdm and ctf gamemodes and the grenade and laser instagib modes for them, as well as the iFreeze gamemode.  
tw+ server by teetime can be found here: https://github.com/Teetime/teeworlds/tree/tw-plus
## How to build
It can be built like a normal 0.6.3 teeworlds server.  
You may need bam 0.4.0 which can be downloaded from: https://www.teeworlds.com/files/bam-0.4.0.zip  
Using bam, use build target server_debug.  
<code>./bam-0.4.0/bam server_debug</code>  
## Features
Apart from the features of the standard server as well as the tw+ server by teetime, the server include the following:  

**Map**  
multiple flag stands per map (one stand is chosen randomly)  
teleports (player, flag, and projectiles, (todo: laser?))  
grenade fountains  
slow killing death / slow healing / slow armor zone
No-flag zone

**Weapons**  
ninja (toggle invincibility, crazy-weapons, slash speed, heart/armor bonusses on pickup)  
toggleable silly custom weapons (laser->plasmagun, grenade->(wip)bouncy grenade, shotgun->explosive shotgun)  
pistol autoshooting  
(wip)hooking tee into spikes count as kill  

**Other**  
shutdown message  
slash chat commands edited/added, including /me, /w, and /help  
Spectator stays spectator after map change  

**Included Maps**  
included extra maps to show the new features:  
dm_island (slowdeath zone)  
dm_tele (teleports)  
dm_gfountain (teleports, grenade fountain)  
ctf_tele (teleports)  
ctf_sixflags (multiple flag stands)  
ctf_quadflags (multiple flag stands)  
