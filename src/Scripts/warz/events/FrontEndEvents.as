package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the FrontEnd that may or may not be specific to a single screen
	 */
	
	public class FrontEndEvents extends Events
	{
		public static function eventOpenURL(urlID:String)
		{
			send("eventOpenURL", urlID);
		}
		public static function eventRenameCharacter(newName:String)
		{
			send("eventRenameCharacter", newName);
		}
		public static function eventRequestShowDonateGCtoServer(serverID:int)
		{
			send("eventRequestShowDonateGCtoServer", serverID);
		}
		public static function eventDonateGCtoServerCallback(numHours:int, serverID:int)
		{
			send("eventDonateGCtoServerCallback", numHours, serverID);
		}
		public static function eventDonateGCtoServer(numHours:int, serverID:int)
		{
			send("eventDonateGCtoServer", numHours, serverID);
		}
		
		public static function eventMarketplaceActive()
		{
			send("eventMarketplaceActive");
		}
		public static function eventBuyPremiumAccount()
		{
			send("eventBuyPremiumAccount");
		}
		public static function eventTrialUpgradeAccount(code:String)
		{
			send("eventTrialUpgradeAccount", code);
		}		
		public static function eventTrialRequestUpgrade()
		{
			send("eventTrialRequestUpgrade");
		}
		public static function eventShowSurvivorsMap()
		{
			send("eventShowSurvivorsMap");
		}
		
		public static function eventSetCurrentBrowseChannel(channel:uint, quickplay:Boolean)
		{
			send("eventSetCurrentBrowseChannel", channel, quickplay);
		}
		public static function eventChangeOutfit(slotID:uint, newHead:uint, newBody:uint, newLegs:uint)
		{
			send("eventChangeOutfit", slotID, newHead, newBody, newLegs);
		}
		public static function eventLearnSkill(skillID:uint)
		{
			send("eventLearnSkill", skillID);
		}
		public static function eventMyServerUpdateSettings(serverID:uint, newPassword:String, 
														   nameplate:int, crosshair:int, tracers:int, newTimeLimit:uint,
														   trialsAllowed:int, disableASR:int, disableSNP:int)
		{
			send("eventMyServerUpdateSettings", serverID, newPassword, nameplate, crosshair, tracers, newTimeLimit, trialsAllowed, disableASR, disableSNP);
		}
		public static function eventMyServerJoinServer(serverID:uint)
		{
			send("eventMyServerJoinServer", serverID);
		}
		public static function eventRenewServer(serverID:uint, rentID:int)
		{
			send("eventRenewServer", serverID, rentID);
		}
		public static function eventRenewServerUpdatePrice(serverID:uint, rentID:int)
		{
			send("eventRenewServerUpdatePrice", serverID, rentID);
		}
		public static function eventRentServer(isGameServer:int, mapID:int, regionID:int, slotID:int, rentID:int, name:String, password:String, pveID:int, nameplates:int, crosshair:int, tracers:int)
		{
			send("eventRentServer", isGameServer, mapID, regionID, slotID, rentID, name, password, pveID, nameplates, crosshair, tracers);			
		}
		public static function eventRentServerUpdatePrice(isGameServer:int, mapID:int, regionID:int, slotID:int, rentID:int, name:String, password:String, pveID:int, nameplates:int, crosshair:int, tracers:int)
		{
			send("eventRentServerUpdatePrice", isGameServer, mapID, regionID, slotID, rentID, name, password, pveID, nameplates, crosshair, tracers);			
		}
		public static function eventMyServerKickPlayer(serverID:uint, playerName:String)
		{
			send("eventMyServerKickPlayer", serverID, playerName);
		}
		public static function eventRequestMyServerInfo(serverID:uint)
		{
			send("eventRequestMyServerInfo", serverID);
		}
		public static function eventRequestMyServerList()
		{
			send("eventRequestMyServerList");
		}
		
		public static function eventRequestGCTransactionData()
		{
			send("eventRequestGCTransactionData");
		}
		
		public static function eventRequestLeaderboardData(type:int, board:int, mode:int)
		{
			send("eventRequestLeaderboardData", type, board, mode);
		}
				
		public static function eventStorePurchaseGPRequest()
		{
			send("eventStorePurchaseGPRequest");
		}		
		public static function eventStorePurchaseGPCallback(price:int)
		{
			send("eventStorePurchaseGPCallback", price);
		}
		public static function eventStorePurchaseGDCallback(price:int)
		{
			send("eventStorePurchaseGDCallback", price);
		}
		public static function eventStorePurchaseGP(price:int)
		{
			send("eventStorePurchaseGP", price);
		}		
		public static function eventStorePurchaseGD(price:int)
		{
			send("eventStorePurchaseGD", price);
		}		
		public static function eventClanApplyToJoin(clanID:uint, desc:String)
		{
			send("eventClanApplyToJoin", clanID, desc);
		}
		public static function eventClanBuySlots(idx:uint)
		{
			send("eventClanBuySlots", idx);
		}
		public static function eventClanRespondToInvite(inviteID:uint, accept:Boolean)
		{
			send("eventClanRespondToInvite", inviteID, accept);
		}
		public static function eventClanInviteToClan(charname:String)
		{
			send("eventClanInviteToClan", charname);
		}
		public static function eventClanApplicationAction(appID:uint, accepted:Boolean)
		{
			send("eventClanApplicationAction", appID, accepted);
		}
		public static function eventClanDonateGCToClan(amount:uint)
		{
			send("eventClanDonateGCToClan", amount);
		}
		public static function eventClanLeaveClan()
		{
			send("eventClanLeaveClan");
		}
		public static function eventClanAdminAction(charID:uint, actionType:String)
		{
			send("eventClanAdminAction", charID, actionType);
		}
		public static function eventClanAdminDonateGC(charID:uint, numGC:uint)
		{
			send("eventClanAdminDonateGC", charID, numGC);
		}
		public static function eventRequestClanApplications()
		{
			send("eventRequestClanApplications");
		}
		public static function eventCreateClan(name:String, tag:String, desc:String, nameColor:int, tagColor:int, iconID:int)
		{
			send("eventCreateClan", name, tag, desc, nameColor, tagColor, iconID);
		}
		public static function eventRequestMyClanInfo()
		{
			send("eventRequestMyClanInfo");
		}
		public static function eventRequestClanList(sort:int, mode:int)
		{
			send("eventRequestClanList", sort, mode);
		}
		
		public static function eventBrowseGamesRequestFilterStatus()
		{
			send("eventBrowseGamesRequestFilterStatus");
		}
		public static function eventBrowseGamesSetFilter(regus:Boolean, regeu:Boolean, regru:Boolean, regsa:Boolean,
											  filt_gw:Boolean, filt_sh:Boolean, filt_empt:Boolean, filt_full:Boolean, filt_private:Boolean,
											  opt_trac:Boolean, opt_nm:Boolean, opt_ch:Boolean,
											  nameFilter:String, opt_enable:Boolean, opt_pass:Boolean,
											  timeLimit:uint)
		{
			send("eventBrowseGamesSetFilter", regus, regeu, regru, regsa, filt_gw, filt_sh, filt_empt, filt_full, filt_private, opt_trac, opt_nm, opt_ch, nameFilter,opt_enable,opt_pass, timeLimit); 
		}
		public static function eventBrowseGamesJoin(gameID:int)
		{
			send("eventBrowseGamesJoin", gameID);
		}
		public static function eventBrowseGamesOnAddToFavorites(gameID:int)
		{
			send("eventBrowseGamesOnAddToFavorites", gameID);
		}
		public static function eventBrowseGamesRequestList(type:String, sort:String, order:int, oper:int)
		{
			send("eventBrowseGamesRequestList", type, sort, order, oper);
		}
														   
		public static function eventPlayGame()
		{
			send("eventPlayGame");
		}
		public static function eventCancelQuickGameSearch()
		{
			send("eventCancelQuickGameSearch");
		}
		public static function eventQuitGame()
		{
			send("eventQuitGame");
		}
		public static function eventCreateCharacter(gamertag:String, heroID:int, hardcore:int, head:int, body:int, legs:int )
		{
			send("eventCreateCharacter", gamertag, heroID, hardcore, head, body, legs);
		}
		public static function eventDeleteChar ()
		{
			send("eventDeleteChar");
		}
		public static function eventReviveChar ()
		{
			send("eventReviveChar");
		}
		public static function eventReviveCharMoney ()
		{
			send("eventReviveCharMoney");
		}
		public static function eventBuyItem (itemID:uint, price:int, priceGD:int)
		{
			send("eventBuyItem", itemID, price, priceGD);
		}
		public static function eventBackpackFromInventory (inventoryID:uint, gridTo:int, amount:int)
		{
			send("eventBackpackFromInventory", inventoryID, gridTo, amount);
		}
		public static function eventBackpackToInventory (gridFrom:int, amount:int)
		{
			send("eventBackpackToInventory", gridFrom, amount);
		}
		public static function eventBackpackGridSwap (gridFrom:int, gridTo:int)
		{
			send("eventBackpackGridSwap", gridFrom, gridTo);
		}		
		public static function eventSetSelectedChar (slotID:int)
		{
			send("eventSetSelectedChar", slotID);
		}		
		public static function eventOptionsLanguageSelection (language:String)
		{
			send("eventOptionsLanguageSelection", language);
		}		
		public static function eventOptionsReset ()
		{
			send("eventOptionsReset");
		}		
		public static function eventOptionsApply(scr:String, overQ:Number, bright:Number, contr:Number, sound:Number, music:Number,
												 CommV:Number, hintS:Number, RedBlood:Number, AA:Number, SSAO:Number, TerrainD:Number,
												 TerrainT:Number, WaterQ:Number, ShadowQ:Number, LightQ:Number, EffectsQ:Number,
												 MeshD:Number, Aniso:Number, PostProc:Number, MeshT:Number, fullscreen:Number, vsync:Number):void
		{
			send("eventOptionsApply", scr, overQ, bright, contr, sound, music, CommV, hintS, RedBlood, AA, SSAO, TerrainD,
									TerrainT, WaterQ, ShadowQ, LightQ, EffectsQ, MeshD, Aniso, PostProc, MeshT, fullscreen, vsync);
		}
		public static function eventRequestPlayerRender (index:int)
		{
			send("eventRequestPlayerRender", index);
		}		
		public static function eventOptionsControlsRequestKeyRemap(index:Number)
		{
			send("eventOptionsControlsRequestKeyRemap", index);
		}
		public static function eventOptionsControlsApply(vertLook:Number, leftSideCam:Number, mouseWheel:Number, sens:Number, accel:Number, toggleAim:Number, toggleCrouch:Number):void
		{
			send("eventOptionsControlsApply", vertLook, leftSideCam, mouseWheel, sens, accel, toggleAim, toggleCrouch);
		}
		public static function eventOptionsControlsReset():void
		{
			send("eventOptionsControlsReset");
		}
		public static function eventOptionsVoipApply(enableVoiceChat:Number, showChatBubble:Number, inputDevice:Number, outputDevice:Number, voiceChatVolume:Number):void
		{
			send("eventOptionsVoipApply", enableVoiceChat, showChatBubble, inputDevice, outputDevice, voiceChatVolume);
		}
		public static function eventOptionsVoipReset():void
		{
			send("eventOptionsVoipReset");
		}
		
		public static function eventCreateChangeCharacter(heroID:int, head:int, body:int, legs:int)
		{
			send("eventCreateChangeCharacter", heroID, head, body, legs);
		}
		public static function eventCreateCancel()
		{
			send("eventCreateCancel");
		}
		public static function eventMsgBoxCallback ()
		{
			send ("eventMsgBoxCallback");
		}
		public static function eventMsgBoxCancelCallback ()
		{
			send ("eventMsgBoxCancelCallback");
		}
		public static function eventChangeBackpack (slotID:int, itemID:uint)
		{
			send ("eventChangeBackpack", slotID, itemID);
		}
		public static function eventOpenBackpackSelector ()
		{
			send ("eventOpenBackpackSelector");
		}
	}
}