package warz.frontend {
	import flash.display.MovieClip;
	import warz.dataObjects.*;
	import flash.events.*;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import caurina.transitions.Tweener;

	public class MyServersScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var MyServers:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;
		private var ScrollerIsDragging2:Boolean;
		private var Scroller2:MovieClip;
		
		public var ServerList:Array = null;
		public var SelectedServerID:uint = 0;
		public var PlayerList:Array = null;
		
		public	function MyServersScreen ()
		{
			visible = false;
			var me = this;
			
			Scroller = MyServers.ServerListScroller;
			ScrollerIsDragging = false;
			Scroller.Field.alpha = 0.5;
			Scroller.ArrowUp.alpha = 0.5;
			Scroller.ArrowDown.alpha = 0.5;
			Scroller.Trigger.alpha = 0.5;

			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_OVER, function() {if(me.ScrollerIsDragging==true) return; me.Scroller.Trigger.alpha = 1; })
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_OUT, function() {if(me.ScrollerIsDragging==true) return; me.Scroller.Trigger.alpha = 0.5; })
			Scroller.ArrowUp.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller.ArrowUp.alpha = 1;})
			Scroller.ArrowUp.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller.ArrowUp.alpha = 0.5;})
			Scroller.ArrowUp.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown(-10);})
			Scroller.ArrowDown.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller.ArrowDown.alpha = 1;})
			Scroller.ArrowDown.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller.ArrowDown.alpha = 0.5; })
			Scroller.ArrowDown.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown(+10);})
			
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function() 
			{
				var startY = me.Scroller.Field.y;
				var endY = me.Scroller.Field.height - me.Scroller.Trigger.height;
				me.ScrollerIsDragging = true;
				me.Scroller.Trigger.startDrag(false, new Rectangle (me.Scroller.Trigger.x, startY, 0, endY));
			})

			Scroller2 = MyServers.PlayersListScroller;
			ScrollerIsDragging2 = false;
			Scroller2.Field.alpha = 0.5;
			Scroller2.ArrowUp.alpha = 0.5;
			Scroller2.ArrowDown.alpha = 0.5;
			Scroller2.Trigger.alpha = 0.5;

			Scroller2.Trigger.addEventListener(MouseEvent.MOUSE_OVER, function() {if(me.ScrollerIsDragging2==true) return; me.Scroller2.Trigger.alpha = 1; })
			Scroller2.Trigger.addEventListener(MouseEvent.MOUSE_OUT, function() {if(me.ScrollerIsDragging2==true) return; me.Scroller2.Trigger.alpha = 0.5; })
			Scroller2.ArrowUp.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller2.ArrowUp.alpha = 1;})
			Scroller2.ArrowUp.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller2.ArrowUp.alpha = 0.5;})
			Scroller2.ArrowUp.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown2(-10);})
			Scroller2.ArrowDown.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller2.ArrowDown.alpha = 1;})
			Scroller2.ArrowDown.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller2.ArrowDown.alpha = 0.5; })
			Scroller2.ArrowDown.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown2(+10);})
			
			Scroller2.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function() 
			{
				var startY = me.Scroller2.Field.y;
				var endY = me.Scroller2.Field.height - me.Scroller2.Trigger.height;
				me.ScrollerIsDragging2 = true;
				me.Scroller2.Trigger.startDrag(false, new Rectangle (me.Scroller2.Trigger.x, startY, 0, endY));
			})

			MyServers.BtnRentStronghold.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyServers.BtnRentStronghold.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyServers.BtnRentStronghold.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			MyServers.BtnRentStronghold.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			MyServers.BtnRentStronghold.Text.Text.text = "$FR_RentStronghold";
			
			MyServers.BtnRent.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyServers.BtnRent.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyServers.BtnRent.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			MyServers.BtnRent.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			MyServers.BtnRent.Text.Text.text = "$FR_RentGameserver";
			
			MyServers.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyServers.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyServers.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			MyServers.BtnBack.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			MyServers.BtnBack.Text.Text.text = "$FR_Back";

			MyServers.BtnJoinServer.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyServers.BtnJoinServer.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyServers.BtnJoinServer.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			MyServers.BtnJoinServer.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);

			MyServers.Settings.BtnSave.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyServers.Settings.BtnSave.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyServers.Settings.BtnSave.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			MyServers.Settings.BtnSave.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			MyServers.Settings.BtnSave.Text.Text.text = "$FR_SaveSettings";

			MyServers.Settings.BtnSave.ActionFunction = function()
			{
				FrontEndEvents.eventMyServerUpdateSettings(SelectedServerID, MyServers.Settings.Password.text,
														   MyServers.Settings.Checkbox1.isSelected,
														   MyServers.Settings.Checkbox2.isSelected,
														   MyServers.Settings.Checkbox3.isSelected,
														   uint(MyServers.Settings.GametimeLimit.text),
														   MyServers.Settings.Checkbox4.isSelected,
														   MyServers.Settings.Checkbox5.isSelected,
														   MyServers.Settings.Checkbox6.isSelected);
				MyServers.Settings.Password.text = "";
			}

			MyServers.BtnRentStronghold.ActionFunction = function()
			{
				api.Main.showScreen("RentStrongholdServer");
				//api.showInfoMsg("$FR_ComingSoon", true);
			}
			MyServers.BtnRent.ActionFunction = function()
			{
				api.Main.showScreen("RentGameServer");
			}
			MyServers.BtnBack.ActionFunction = function()
			{
				api.Main.showScreen("PlayGame");
			}
			MyServers.BtnJoinServer.ActionFunction = function()
			{
				FrontEndEvents.eventMyServerJoinServer(SelectedServerID);
			}
			
			MyServers.Settings.CheckboxText1.text = "$FR_ShowNameplates";
			MyServers.Settings.CheckboxText2.text = "$FR_ShowCrosshair";
			MyServers.Settings.CheckboxText3.text = "$FR_ShowTracers";
			MyServers.Settings.CheckboxText4.text = "$FR_AllowTrials";
			MyServers.Settings.CheckboxText5.text = "$FR_DisableASR";
			MyServers.Settings.CheckboxText6.text = "$FR_DisableSNP";

			MyServers.Settings.Checkbox1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyServers.Settings.Checkbox1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyServers.Settings.Checkbox1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyServers.Settings.Checkbox1.ActionFunction = function(param:int)
			{
				this.isSelected = param;
			}
			MyServers.Settings.Checkbox2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyServers.Settings.Checkbox2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyServers.Settings.Checkbox2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyServers.Settings.Checkbox2.ActionFunction = function(param:int)
			{
				this.isSelected = param;
			}
			MyServers.Settings.Checkbox3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyServers.Settings.Checkbox3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyServers.Settings.Checkbox3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyServers.Settings.Checkbox3.ActionFunction = function(param:int)
			{
				this.isSelected = param;
			}
			MyServers.Settings.Checkbox4.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyServers.Settings.Checkbox4.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyServers.Settings.Checkbox4.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyServers.Settings.Checkbox4.ActionFunction = function(param:int)
			{
				this.isSelected = param;
			}
			MyServers.Settings.Checkbox5.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyServers.Settings.Checkbox5.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyServers.Settings.Checkbox5.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyServers.Settings.Checkbox5.ActionFunction = function(param:int)
			{
				this.isSelected = param;
			}
			MyServers.Settings.Checkbox6.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyServers.Settings.Checkbox6.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyServers.Settings.Checkbox6.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyServers.Settings.Checkbox6.ActionFunction = function(param:int)
			{
				this.isSelected = param;
			}
		}
		
		private function BtnRollOverFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		private function BtnRollOutFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		private function BtnPressFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.State = "active";
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction(1);
			}
			else
			{
				evt.currentTarget.parent.State = "none";
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.gotoAndPlay("out");
				evt.currentTarget.parent.ActionFunction(0);
			}
		}
		
		private function BtnRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
		}
		private function BtnRollOutFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		private function BtnPressFn(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
			evt.currentTarget.parent.gotoAndPlay("pressed");
		}
		private function BtnPressUpFn(evt:Event)
		{
			evt.currentTarget.parent.ActionFunction();
		}

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			{
				while(MyServers.ServerList.numChildren > 0)
					MyServers.ServerList.removeChildAt(0);
				while(MyServers.PlayersList.numChildren > 0)
					MyServers.PlayersList.removeChildAt(0);
			}
			ServerList = new Array();
			FrontEndEvents.eventRequestMyServerList();
			
			MyServers.BtnJoinServer.visible = false;
			MyServers.Settings.visible = false;
			MyServers.PlayersListScroller.visible = false;

			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
			this.visible = true;

			if(api.isDebug)
			{
				addServerInfo(0, "first", "east", "us", "online", 50, 100, "99D 45H 53M", "GAME SERVER", "COLORADO", true);
				for(var i=1; i<10; ++i)
					addServerInfo(i, "middle", "west", "ru", "offline", 10, 500, "0D 45H 53M", "STRONGHOLD", "CLIFFSIDE", false);
				addServerInfo(100, "last", "europe", "eu", "online", 100, 100, "99D 45H 53M", "GAME SERVER", "COLORADO", true);
				
				showServerList();
			}
		}
		
		public function Deactivate()
		{
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);
			this.visible = false;
		}
		
		public function addServerInfo(serverID:uint, name:String, location:String, region:String, status:String, playersOnline:uint, maxPlayers:uint, timeLeft:String, type:String, map:String, renew:Boolean)
		{
			ServerList.push({serverID:serverID, name:name, location:location, region:region, status:status, playersOnline:playersOnline, maxPlayers:maxPlayers, timeLeft:timeLeft, type:type, map:map, movie:null, renew:renew});			
		}
		
		public function updatePlayerOnlineForServerID(serverID:uint, plrOnline:uint)
		{
			for(var i=0; i<ServerList.length; ++i)
			{
				if(ServerList[i]["serverID"]==serverID)
				{
					ServerList[i]["playersOnline"] = plrOnline;
					ServerList[i]["movie"].Text.PlayersOnline.text = ServerList[i]["playersOnline"]+"/"+ServerList[i]["maxPlayers"];
					break;
				}
			}
		}
		
		public function showServerList()
		{
			var me = this;
			
			while(MyServers.ServerList.numChildren > 0)
				MyServers.ServerList.removeChildAt(0);
			
			var slotY = 0;
			for(var i=0; i<ServerList.length; ++i)
			{
				var slot = new warz.frontend.PGMyServersListSlot();
				ServerList[i]["movie"] = slot;

				slot.x = 0;
				slot.y = slotY; slotY += 70;
				
				slot.serverID = ServerList[i]["serverID"];	
				slot.Text.ServerName.text = ServerList[i]["name"];
				slot.Text.Location.text = ServerList[i]["location"];
				slot.Text.Status.text = ServerList[i]["status"];
				slot.Text.PlayersOnline.text = ServerList[i]["playersOnline"]+"/"+ServerList[i]["maxPlayers"];
				slot.Text.ExpiresIn.text = ServerList[i]["timeLeft"];
				slot.Text.Map.text = ServerList[i]["map"];
				slot.Text.Type.text = ServerList[i]["type"];
				
				slot.Flags.gotoAndStop(ServerList[i]["region"]);
				
				slot.BtnRenew.Text.Text.text = "$FR_Renew";
				slot.BtnRenew.visible = ServerList[i]["renew"];;
				slot.BtnRenew.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event)
												 {
													 evt.currentTarget.parent.gotoAndPlay("over");
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.MOUSE_OVER));
												 });
				slot.BtnRenew.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event)
												 {
													 evt.currentTarget.parent.gotoAndPlay("out");
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.MOUSE_OUT));
												 });
				slot.BtnRenew.Btn.addEventListener(MouseEvent.MOUSE_DOWN, function(evt:Event)
												 {
													 SoundEvents.eventSoundPlay("menu_click");
													 evt.currentTarget.parent.gotoAndPlay("pressed");
												 });
				slot.BtnRenew.Btn.addEventListener(MouseEvent.MOUSE_UP, function(evt:Event)
												 {
													 //evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.CLICK));
													 var serverID:int = evt.currentTarget.parent.parent.serverID;
													 api.Main.RentServerPopup.renewServerID = serverID;
													 api.Main.showScreen("RentRenewServer");													 
												 });
				
				slot.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event)
										  {
										  	if(evt.currentTarget.parent.State != "active")
												evt.currentTarget.parent.gotoAndPlay("over");
	  									  });
				slot.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event)
										  {
											if(evt.currentTarget.parent.State != "active")
												evt.currentTarget.parent.gotoAndPlay("out");  
										  });
				slot.Btn.addEventListener(MouseEvent.CLICK, function(evt:Event)
										  {
											  //if(evt.currentTarget.parent.State != "active")
											  {
												  for(var i=0; i<me.ServerList.length; ++i)
												  {
													  if(me.ServerList[i]["movie"].State == "active")
													  {
														  me.ServerList[i]["movie"].State = "none";
														  me.ServerList[i]["movie"].gotoAndPlay("out");
													  }
												  }
												  SoundEvents.eventSoundPlay("menu_click");
												  evt.currentTarget.parent.State = "active";
												  evt.currentTarget.parent.gotoAndPlay("pressed");
												  
												  MyServers.BtnJoinServer.visible = true;
												  MyServers.Settings.visible = false;
												  me.SelectedServerID = evt.currentTarget.parent.serverID;
												  me.PlayerList = new Array();
												  FrontEndEvents.eventRequestMyServerInfo(me.SelectedServerID);
												  if(api.isDebug)
												  {
													  addPlayerInfo("first", "alive", "[bandit]", 500000);
													  for(i=0; i<10; ++i)
													  	addPlayerInfo("middle", "dead", "[lawful]", 100000);
													  addPlayerInfo("last", "alive", "[blablabla]", 99100000);
													  
													  showPlayerList();
												  }
											  }
										  });
				
				MyServers.ServerList.addChild(slot);				
			}
			
			if (ServerList.length > 3)
			{
				Scroller.visible = true;
				
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = MyServers.ServerMask.height - 4;
				Scroller.Field.height = (MyServers.ServerMask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				MyServers.ServerList.y = 273;
			}
		}
		
		public function setServerInfoSettingsVisibility(st:Boolean, nameplate:int, crosshair:int, tracers:int, timeLimit:uint, trialAllow:int, disableASR:int, disableSNP:int)
		{
			MyServers.Settings.GametimeLimit.text = timeLimit;			
			MyServers.Settings.Checkbox1.isSelected = nameplate; 
			if(nameplate)
			{
				MyServers.Settings.Checkbox1.State = "active";
				MyServers.Settings.Checkbox1.gotoAndPlay("pressed");
			}
			else
			{
				MyServers.Settings.Checkbox1.State = "none";
				MyServers.Settings.Checkbox1.gotoAndStop(1);
			}

			MyServers.Settings.Checkbox2.isSelected = crosshair; 
			if(crosshair)
			{
				MyServers.Settings.Checkbox2.State = "active";
				MyServers.Settings.Checkbox2.gotoAndPlay("pressed");
			}
			else
			{
				MyServers.Settings.Checkbox2.State = "none";
				MyServers.Settings.Checkbox2.gotoAndStop(1);
			}

			MyServers.Settings.Checkbox3.isSelected = tracers; 
			if(tracers)
			{
				MyServers.Settings.Checkbox3.State = "active";
				MyServers.Settings.Checkbox3.gotoAndPlay("pressed");
			}
			else
			{
				MyServers.Settings.Checkbox3.State = "none";
				MyServers.Settings.Checkbox3.gotoAndStop(1);
			}

			MyServers.Settings.Checkbox4.isSelected = trialAllow; 
			if(trialAllow)
			{
				MyServers.Settings.Checkbox4.State = "active";
				MyServers.Settings.Checkbox4.gotoAndPlay("pressed");
			}
			else
			{
				MyServers.Settings.Checkbox4.State = "none";
				MyServers.Settings.Checkbox4.gotoAndStop(1);
			}

			MyServers.Settings.Checkbox5.isSelected = trialAllow; 
			if(disableASR)
			{
				MyServers.Settings.Checkbox5.State = "active";
				MyServers.Settings.Checkbox5.gotoAndPlay("pressed");
			}
			else
			{
				MyServers.Settings.Checkbox5.State = "none";
				MyServers.Settings.Checkbox5.gotoAndStop(1);
			}

			MyServers.Settings.Checkbox6.isSelected = trialAllow; 
			if(disableSNP)
			{
				MyServers.Settings.Checkbox6.State = "active";
				MyServers.Settings.Checkbox6.gotoAndPlay("pressed");
			}
			else
			{
				MyServers.Settings.Checkbox6.State = "none";
				MyServers.Settings.Checkbox6.gotoAndStop(1);
			}

			MyServers.Settings.visible = st;
		}
		
		public function addPlayerInfo(name:String, status:String, rep:String, xp:int)
		{
			PlayerList.push({name:name, status:status, rep:rep, xp:xp, movie:null});
		}
		
		public function showPlayerList()
		{
			var me = this;
			
			while(MyServers.PlayersList.numChildren > 0)
				MyServers.PlayersList.removeChildAt(0);
			
			var slotY = 0;
			for(var i=0; i<PlayerList.length; ++i)
			{
				var slot = new warz.frontend.PGMyServersPListSlot();
				PlayerList[i]["movie"] = slot;

				slot.x = 0;
				slot.y = slotY; slotY += 54;
				
				slot.playerID = PlayerList[i]["name"];	
				slot.Text.Username.text = PlayerList[i]["name"];
				slot.Text.Status.text = PlayerList[i]["status"];
				slot.Text.Alignment.text = PlayerList[i]["rep"];
				slot.Text.Score.text = PlayerList[i]["xp"];
				
				slot.Kick.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event)
												 {
													 evt.currentTarget.parent.gotoAndPlay("over");
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.MOUSE_OVER));
												 });
				slot.Kick.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event)
												 {
													 evt.currentTarget.parent.gotoAndPlay("out");
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.MOUSE_OUT));
												 });
				slot.Kick.Btn.addEventListener(MouseEvent.MOUSE_DOWN, function(evt:Event)
												 {
													 SoundEvents.eventSoundPlay("menu_click");
													 evt.currentTarget.parent.gotoAndPlay("pressed");
												 });
				slot.Kick.Btn.addEventListener(MouseEvent.MOUSE_UP, function(evt:Event)
												 {
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.CLICK));
													 var playerName:String = evt.currentTarget.parent.parent.playerID;
													 FrontEndEvents.eventMyServerKickPlayer(SelectedServerID, playerName);
												 });
				
				slot.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event)
										  {
										  	if(evt.currentTarget.parent.State != "active")
												evt.currentTarget.parent.gotoAndPlay("over");
	  									  });
				slot.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event)
										  {
											if(evt.currentTarget.parent.State != "active")
												evt.currentTarget.parent.gotoAndPlay("out");  
										  });
				slot.Btn.addEventListener(MouseEvent.CLICK, function(evt:Event)
										  {
											  if(evt.currentTarget.parent.State != "active")
											  {
												  for(var i=0; i<me.PlayerList.length; ++i)
												  {
													  if(me.PlayerList[i]["movie"].State == "active")
													  {
														  me.PlayerList[i]["movie"].State = "none";
														  me.PlayerList[i]["movie"].gotoAndPlay("out");
													  }
												  }
												  SoundEvents.eventSoundPlay("menu_click");
												  evt.currentTarget.parent.State = "active";
												  evt.currentTarget.parent.gotoAndPlay("pressed");
												  
												  var playerName:String = evt.currentTarget.parent.playerID;
											  }
										  });
				
				MyServers.PlayersList.addChild(slot);				
			}
			
			if (PlayerList.length > 4)
			{
				Scroller2.visible = true;
				
				Scroller2.Trigger.y = Scroller2.Field.y;
				Scroller2.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems2);
				
				Scroller2.ArrowDown.y = MyServers.PlayersMask.height - 4;
				Scroller2.Field.height = (MyServers.PlayersMask.height - (Scroller2.ArrowDown.height * 2));
			}
			else
			{
				Scroller2.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems2);
				Scroller2.visible = false;
				MyServers.PlayersList.y = 769;
			}
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = MyServers.ServerList.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = MyServers.ServerMask.height;
			h -= mh;
			h /= h1;

			var	dest:Number = dist * h;
			var	step:Number = int (dest / 70);
			
			if (MyServers.ServerList.y != (273+(70 * step)))
			{
				Tweener.addTween(MyServers.ServerList, {y:(273+(70 * step)), time:api.tweenDelay, transition:"linear"});
			}
		}

		public	function scrollItems2(e:Event)
		{
			var	a = Scroller2.Field.y;
			var	b = Scroller2.Trigger.y;
			var	dist = (Scroller2.Field.y - Scroller2.Trigger.y);
			var	h = MyServers.PlayersList.height;
			var	h1 = Scroller2.Field.height - Scroller2.Trigger.height;
			var	mh = MyServers.PlayersMask.height;
			h -= mh;
			h /= h1;

			var	dest:Number = dist * h;
			var	step:Number = int (dest / 54);
			
			if (MyServers.PlayersList.y != (769+(54 * step)))
			{
				Tweener.addTween(MyServers.PlayersList, {y:(769+(54 * step)), time:api.tweenDelay, transition:"linear"});
			}
		}

		public	function mouseUp(e:Event) :void
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
			ScrollerIsDragging2 = false;
			Scroller2.Trigger.stopDrag(); 
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > MyServers.ServerMask.x && 
				e.stageX < MyServers.ServerMask.x + MyServers.ServerMask.width + 45 &&
				e.stageY > MyServers.ServerMask.y && 
				e.stageY < MyServers.ServerMask.y + MyServers.ServerMask.height)
			{
				var dist = (MyServers.ServerList.height - MyServers.ServerMask.height) / 70;
				var h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;
				
				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
			}
			if (e.stageX > MyServers.PlayersMask.x && 
				e.stageX < MyServers.PlayersMask.x + MyServers.PlayersMask.width + 45 &&
				e.stageY > MyServers.PlayersMask.y && 
				e.stageY < MyServers.PlayersMask.y + MyServers.PlayersMask.height)
			{
				var dist2 = (MyServers.PlayersList.height - MyServers.PlayersMask.height) / 54;
				var h2 = Scroller2.Field.height - Scroller2.Trigger.height + 26;
				dist2 = h2 / dist2;
				
				if (e.delta > 0)
					handleScrollerUpDown2(-dist2);
				else
					handleScrollerUpDown2(dist2);
			}
		}
		public function handleScrollerUpDown(delta:Number)
		{
			if(delta < 0)
			{ 
				var startY = Scroller.Field.y; 
				if(Scroller.Trigger.y > startY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y < startY) 
						Scroller.Trigger.y = startY; 
				} 
			}
			else
			{
				var endY = Scroller.Field.height - Scroller.Trigger.height+26;
				if(Scroller.Trigger.y < endY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y > endY) 
						Scroller.Trigger.y = endY; 
				} 
			}
		}
		public function handleScrollerUpDown2(delta:Number)
		{
			if(delta < 0)
			{ 
				var startY = Scroller2.Field.y; 
				if(Scroller2.Trigger.y > startY) { 
					Scroller2.Trigger.y += delta; 
					if(Scroller2.Trigger.y < startY) 
						Scroller2.Trigger.y = startY; 
				} 
			}
			else
			{
				var endY = Scroller2.Field.height - Scroller2.Trigger.height+26;
				if(Scroller2.Trigger.y < endY) { 
					Scroller2.Trigger.y += delta; 
					if(Scroller2.Trigger.y > endY) 
						Scroller2.Trigger.y = endY; 
				} 
			}
		}

	}
}