Copyright (c) 2012 Magnus Auvinen


This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.


Please visit http://www.teeworlds.com for up-to-date information about 
the game, including new versions, custom maps and much more.

# Teeworlds+ (TW+)

This modification is mainly directed to Instagib-players but of course vanilla is supported too.
TW+ contains a lot of features which might be useful for one or two and is highly customizable, what means that you can disable features which you dont want or need.

Features:
- New Gametypes (Possible: DM+, TDM+, CTF+, iDM+, iTDM+, iCTF+, iFreeze+, gDM+, gTDM+, gCTF+)
- Mute-On-Spam
- Anti-Capslock
- Anticamper
- Chat-commands to control the game
- Private message
- Spawnprotection
- Logging player stats (useful for wars)
- [Loltext](https://github.com/fstd/teeworlds/tree/f_loltext)
- Message loops for chat
- Killing-spree and killing-spree award
- Live-stats for every player
- Emotional Tees
- Laserjumps
- ... many more

### iFreeze+

As you see it also includes the iFreeze-gametype which was very popular in old 0.5.

The gameplay is similar to (open)FNG and TDM. You have just one weapon; your laser. If you kill a player he becomes frozen and cant move anymore. Your team will score when every tee of the opponent team is frozen, then all frozen players become melted and it starts again.

Of course you can also melt your teammates: When you stand near to a frozen teammate, after some time he becomes melted and can continue playing.
You will **score** both **by freezing** a opponent **and melting** a teammate. It is very important to melt your teammates, otherwise your team will probably loose.

## Server Settings
| Variable | Default | Minimum | Maximum | Description |
| --- | --- | --- | --- | --- |
| sv_vote_forcereason | 1 | 0 | 1 | Allow only votes with a reason (except settings) |
| sv_go_time | 5 | 0 | 600 | The restart time for the go-command |
| sv_stopgo_feature | 1 | 0 | 1 | Enable stop/go-feature in chat |
| sv_xonx_feature | 1 | 0 | 1 | Enable chat-commands like `/1on1` - `/6on6` for wars |
| sv_war_time | 15 | 0 | 600 | Default warmup-time before a war |
| sv_laserjumps | 0 | 0 | 1 | Enable laserjumps (only in instagib) |
| sv_emotional_tees | 1 | 0 | 1 | Enable emotional tees |
| sv_private_message | 1 | 0 | 1 | Enable/Disable private message |
| sv_spawnprotection | 0 | 0 | 5 | Spawnprotection in seconds (0 disables) |
| sv_laser_reload_time | 800 | 0 | 2400 | Reload-time for laser when you are not at killing-spree (Default: 800) |
| sv_chat_value | 250 | 100 | 1000 | A value which is added on each message and decreased on each tick |
| sv_chat_threshold | 1000 | 250 | 10000 | If this threshold will exceed by too many messages the player will be muted |
| sv_mute_duration | 60 | 0 | 3600 | How long the player will be muted (in seconds) |
| sv_chat_max_duplicates | 3 | 0 | 1 | How many duplicates of a chat message is allowed to send in a row (-1 for no limit) |
| sv_vote_mute | 1 | 0 | 1 | Allow voting to mute players |
| sv_vote_mute_duration | 180 | 0 | 600 | How many seconds to mute a player after being muted by vote |
| sv_chat_message | \<none\> | - | - | A message which will be periodically shown in chat |
| sv_chat_message_interval | 15 | 7 | 1000000 | The interval in minutes where the chatmessage is sent to the chat"
| sv_anticapslock | 1 | 0 | 1 | If all letters of a chat-message are capitalized they will be lowercased |
| sv_anticapslock_tolerance | 5 | 0 | 10 | How many letters of 10 are allowed to be lowercased that it just work |
| sv_anticapslock_minimum | 4 | 1 | 15 | Minimum number of letters that the anti-capslock feature works |
| sv_anticamper | 0 | 0 | 2 | 0 disables, 1 enables anticamper in all modes and 2 only in instagib gamemodes |
| sv_anticamper_freeze | 0 | 0 | 15 | If a player should freeze on camping (and how long) or die |
| sv_anticamper_time | 9 | 5 | 20 | How many seconds to wait till the player dies/freezes |
| sv_anticamper_range | 200 | 0 | 1000 | Distance how far away the player must move to escape anticamper |
| sv_stats_file | "stats.txt" | - | - | Name of the file where the statistics are stored in |
| sv_stats_outputlevel | 0 | 0 | 3 | How much informations in the statistics-file should be saved (0 to disable saving) |
| sv_loltext_show | 1 | 0 | 1 | Show loltext |
| sv_loltext_hspace | 14 | 10 | 25 | Horizontal offset between loltext 'pixels' |
| sv_loltext_vspace | 14 | 10 | 25 | Vertical offset between loltext 'pixels' |
| sv_loltext_lifespan | 50 | 25 | 100 | How long the loltext is shown |
| sv_grenade_min_damage | 4 | 3 | 6 | Minimum damage the grenade must make to kill the player (depends how far away the bullet explodes) |
| sv_grenade_ammo | 6 | -1 | 10 | How much ammo for the grenade (-1 for endless) |
| sv_grenade_ammo_regen | 1000 | 800 | 2000 | Time till one bullet regenerates |
| sv_ifreeze_automelt_time | 30 | 10 | 120 | Time till the player respawn automatically when he's frozen |
| sv_ifreeze_melt_range | 100 | 10 | 1000 | Maximum range to melt a player |
| sv_ifreeze_melt_time | 1200 | 500 | 5000 | Time (in ms) a player has to stand next to an frozen player to melt him |
| sv_ifreeze_melt_respawn | 1 | 0 | 1 | If a player respawns after he was being melted |
| sv_ifreeze_frozen_tag | 1 | 0 | 1 | If frozen players have `[F]` in front of their name |
| sv_kspree_kills | 5 | 3 | 20 | How many kills are needed to be on a killing-spree |
| sv_kspree_award | 0 | 0 | 1 | If players with more than `sv_kspree_kill` kills get the killingspree award |
| sv_kspree_ifreeze | 1 | 0 | 1 | Give the killingspree award only in iFreeze |
| sv_kspree_award_laser | 3 | 1 | 10 | How many lasers will shot when you got the killingspree award |
| sv_kspree_award_laser_split | 1 | 1 | 10 | Split of the lasers while having the killingspree award |
| sv_kspree_award_laser_firedelay | 100 | 0 | 800 | Firedelay of the weapon when you got the killingspree award |

## Rcon-Commands

_Note: The letters behind a command determines what kind of argument and how many are required. i = number, r = string. A leading questionmark (?) means the following argument is optional._

| Command | Usage | Description |
| --- | --- | --- |
| freeze ii | freeze &lt;ClientID&gt; &lt;Time in sec&gt; | Freeze a player for x seconds |
| unfreeze i | unfreeze &lt;ClientID&gt; | Unfreeze a player |
| melt i | melt &lt;ClientID&gt; | Melt a player (same effect like unfreeze) |
| mute ii | mute &lt;ClientID&gt; &lt;Time in sec&gt; | Mute a player for x sec |
| unmuteid i | unmute &lt;ClientID&gt; | Unmutes a player by its client id |
| unmuteip i | unmuteip &lt;Index&gt; | Remove a mute by its index (see &quot;mutes&quot;) |
| mutes | mutes | Show all mutes |
| mute_spec ?i | mute_spec &lt;`0` or `1` or nothing&gt; | All messages written from spectators to the public chat will be redirect to their teamchat for this round (_Note: Useful as callvote to avoid annoying messages_) |
| stop | stop | Pause the game |
| go ?i | go &lt;Time in sec or nothing&gt; | Continue the game |
| set_name ir | set_name &lt;ClientID&gt; &lt;New Name&gt; | Set the name of a player |
| set_clan ir | set_clan &lt;ClientID&gt; &lt;New Clanname&gt; | Set the clan of a player |
| kill i | kill &lt;ClientID&gt; | Kills a player |
| whois | whois | Show a list of players which are currently logged in in rcon |

## Chat-Commands
_Note: Alternatives are seperated by a comma. For `go`, `stop` and `restart` it's not necessary to type a leading slash._

| Command | Usage | Description |
| --- | --- | --- |
|/info | /info | Show information about this mod |
| /credits | /credits | Show some credits... |
| /stats | /stats &lt;Playername&gt; | Shows statistics of any player. You dont need to write your own name for your own statistics |
| /sayto , /st , /pm | /sayto &lt;Name/ID&gt; &lt;Message&gt; | Send a private message to a player in the server|
| /ans , /r | /ans &lt;Message&gt;  | Answer to a private message. The receiver is the player from where the last message was received or sent |
| /emote | /emote &lt;Emotename&gt; &lt;Time in sec&gt; | Set an emote. Available are `surprise`, `blink`, `happy`, `pain`, `angry` and `normal` |
| /stop | /stop | Pause the game |
| /go | /go | Resume the game |
| /restart | /restart | Restart the game |
| /1on1 - /6on6 | /1on1 | Start a war and set spectator slots |
| /reset | /reset | Reset spectator slots which may have been set by `\1on1` - `\6on6` |

For other server settings please see the official [Teeworlds documentation](https://www.teeworlds.com/?page=docs&wiki=docs_overview)

---

#### Current Version: 0.7.5

#### [Download](https://github.com/Teetime/teeworlds/releases)

---

**Example Configs:**
- [iCTF\+](https://github.com/Teetime/teeworlds/blob/tw-plus/sample-ictf.cfg)
- [iFreeze\+](https://github.com/Teetime/teeworlds/blob/tw-plus/sample-ifreeze.cfg)
