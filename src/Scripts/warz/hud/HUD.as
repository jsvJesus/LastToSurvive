package warz.hud {
	import flash.display.MovieClip;
	import caurina.transitions.Tweener;
	import flash.geom.Rectangle;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import flash.utils.setTimeout;
	import warz.events.HUDEvents;
	import warz.events.SoundEvents;
	import flash.events.KeyboardEvent;
	import flash.geom.ColorTransform;
	import scaleform.gfx.*;
	import flash.geom.Point;
	import scaleform.gfx.MouseEventEx;
	
	public class HUD extends MovieClip {
		public var isDebug:Boolean = false;
		
		public var Main:MovieClip = null;

		static public var api:HUD = null;

		public	var	activeChatTab:int = 0;

		public	var		showchat:Boolean = true;
		public	var		showInput:Boolean = true;
		
		public	var 	Players:Array = new Array();
		
		private var 	PlayerList_CurrentPlayerSelected:String = "";
		
		public var		GroupPlayers:Array = new Array();
		
		public var 		VOIPPlayers:Array = new Array();
		
		public var		MissionList:Array = new Array();

		private var ScrollerIsDragging:Boolean;
		public var Scroller:MovieClip;

		public function HUD(main:MovieClip) 
		{
			api = this;
			Main = main;
			Main.init();
			
			Main.tabEnabled = false;
			Main.Chat.Chat.tabEnabled = false;
			Main.Chat.Chat.InputText.tabEnabled = false;
			//TextFieldEx.setNoTranslate(Main.Chat.Chat.InputText, true);
			
			Main.VoipIcon.visible = false;
			
			Main.PlayerList_Dropmenu.Btn1.Text.Text.text = "";
			Main.PlayerList_Dropmenu.Btn2.Text.Text.text = "";
			Main.PlayerList_Dropmenu.Btn3.Text.Text.text = "";
			Main.PlayerList_Dropmenu.Btn4.Text.Text.text = "";
			Main.PlayerList_Dropmenu.Btn5.Text.Text.text = "";
			Main.PlayerList_Dropmenu.Btn6.Text.Text.text = "";
			Main.PlayerList_Dropmenu.Btn7.Text.Text.text = "";

			for(var i=1; i<=7; ++i)
			{
				var Name = "Btn"+i;
				Main.PlayerList_Dropmenu[Name].actionID = i;
				Main.PlayerList_Dropmenu[Name].Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event)  {
												evt.currentTarget.parent.gotoAndPlay("over");   });
				Main.PlayerList_Dropmenu[Name].Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event)  {
												evt.currentTarget.parent.gotoAndPlay("out");  });
				Main.PlayerList_Dropmenu[Name].Btn.addEventListener(MouseEvent.CLICK, function(evt:Event) 
											  {
												  SoundEvents.eventSoundPlay("menu_click");
												  evt.currentTarget.parent.gotoAndPlay("pressed");
												  HUDEvents.eventPlayerListAction(evt.currentTarget.parent.actionID, PlayerList_CurrentPlayerSelected);
											  });
			}
			
			for(i=1; i<=10; ++i)
			{
				Name = "Slot"+i;
				Main.GroupMenu[Name].visible = false;
				Main.GroupMenu[Name].Num.text = i;
				Main.GroupMenu[Name].NumShad.text = i;
			}
			Main.GroupMenu.visible = false;
			
			Main.MissionActions.Actions.List.lastY = 0;
			
			Main.CarUI.visible = false;
			Main.CarUI.TitleKMH.text = "$HUD_VehicleSpeedMeasure";
			
			Main.CaliTestText.visible = false;
			
			Tweener.init();
			
			var me = this;
			Scroller = Main.PlayersList.Scroller;
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
				me.Main.PlayerList_Dropmenu.visible = false;
				me.Scroller.Trigger.startDrag(false, new Rectangle (me.Scroller.Trigger.x, startY, 0, endY));
			})
			
			Main.PlayerList_Dropmenu.visible = false;
			Main.PlayersList.visible = false;
		}

		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			var	dist:Number;
			var	h;
			
			trace("stageX="+e.stageX+", maskX="+Main.PlayersList.Mask.x);
			if (e.stageX > Main.PlayersList.Mask.x && 
				e.stageX < Main.PlayersList.Mask.x + Main.PlayersList.Mask.width + 45 &&
				e.stageY > Main.PlayersList.Mask.y && 
				e.stageY < Main.PlayersList.Mask.y + Main.PlayersList.Mask.height)
			{
				dist = (Main.PlayersList.List.height - Main.PlayersList.Mask.height) / 42;
				h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;
				
				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
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
				var endY = Scroller.Field.height - Scroller.Trigger.height+25;
				if(Scroller.Trigger.y < endY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y > endY) 
						Scroller.Trigger.y = endY; 
				} 
			}
		}

		// type: buggy, stryker, truck
		public function setCarTypeInfo(type:String)
		{
			Main.CarUI.Vehicles.gotoAndStop(type);			
		}
		// type: player, team, clan, filled, empty
		public function setCarSeatInfo(seatID:int, type:String)
		{
			var n = "Seat"+(seatID+1);
			Main.CarUI.Vehicles[n].gotoAndStop(type);
		}

		// durability: [1,100], speed[1,100], speedText:int, gas: [1,100], rpm: [1,100]
		public function setCarInfo(durability:int, speed:int, speedText:int, gas:int, rpm:int)
		{
			Main.CarUI.TextSpeed.text = speedText;
			Main.CarUI.Speed.gotoAndStop(101-speed);
			Main.CarUI.Arrow.gotoAndStop(101-speed);
			
			Main.CarUI.Life.gotoAndStop(101-durability);
			Main.CarUI.Fuel.gotoAndStop(101-gas);
			Main.CarUI.RPM.gotoAndStop(101-rpm);
		}
		
		public function setCarInfoVisibility(vis:Boolean)
		{
			Main.CarUI.visible = vis;
		}
		
		public function setChatVisibility(show:Boolean)
		{
			Main.Chat.visible = show;
		}
		
		public function showMissionInfo(show:Boolean)
		{
			if(show)
				Main.MissionActions.gotoAndPlay("in");
			else
				Main.MissionActions.gotoAndPlay("out");
		}
		
		public function addMissionInfo(name:String)
		{
			//Main.MissionActions.Actions.List
			//warz.hud.MissActSlotTitle
			//warz.hud.MissActSlot
			
			var missionTitle = new warz.hud.MissActSlotTitle();
			missionTitle.Text.text = name;
			missionTitle.x = -0.5;
			missionTitle.y = Main.MissionActions.Actions.List.lastY;
			Main.MissionActions.Actions.List.lastY += 33;
			
			Main.MissionActions.Actions.List.addChild(missionTitle);
						
			MissionList.push({objectives:new Array(), movie:missionTitle});
		}
		public function addMissionObjective(index:uint, name:String, isCompleted:Boolean, Numbers:String, isAreaMarkerVisible:Boolean)
		{
			if(index >= MissionList.length)
				return;
			
			var obj = new warz.hud.MissActSlot();
			obj.x = 0;
			obj.y = Main.MissionActions.Actions.List.lastY;
			Main.MissionActions.Actions.List.lastY += 33;
			if(isCompleted)
				obj.gotoAndPlay("completed");
			obj.Text.text = name;
			if(!isCompleted)
			{	obj.Num.text = Numbers;
				obj.IconsArea.visible = isAreaMarkerVisible;
			}
			
			Main.MissionActions.Actions.List.addChild(obj);
			
			MissionList[index]["objectives"].push({movie:obj});
		}
		public function setMissionObjectiveInArea(mindex:uint, oindex:uint, isInArea:Boolean)
		{
			if(mindex >= MissionList.length)
				return;
			if(oindex >= (MissionList[mindex]["objectives"]).length)
				return;
			(MissionList[mindex]["objectives"])[oindex]["movie"].IconsArea.gotoAndStop(isInArea?"in":"out");
		}
		public function setMissionObjectiveNumbers(mindex:uint, oindex:uint, Num:String)
		{
			if(mindex >= MissionList.length)
				return;
			if(oindex >= (MissionList[mindex]["objectives"])[oindex].length)
				return;
			(MissionList[mindex]["objectives"])[oindex]["movie"].Num.text = Num;
		}
		public function setMissionObjectiveCompleted(mindex:uint, oindex:uint)
		{
			if(mindex >= MissionList.length)
				return;
			if(oindex >= (MissionList[mindex]["objectives"]).length)
				return;
			var tempText = (MissionList[mindex]["objectives"])[oindex]["movie"].Text.text;
			(MissionList[mindex]["objectives"])[oindex]["movie"].gotoAndPlay("completed");
			(MissionList[mindex]["objectives"])[oindex]["movie"].Text.text = tempText;
		}
		public function removeMissionInfo(index:uint)
		{
			if(index >= MissionList.length)
				return;
			for(var i=0; i<MissionList[index]["objectives"].length; ++i)
			{
				Main.MissionActions.Actions.List.removeChild(MissionList[index]["objectives"][i]["movie"]);
			}
			Main.MissionActions.Actions.List.removeChild(MissionList[index]["movie"]);
			var lY = 0;
			for(i=0; i<Main.MissionActions.Actions.List.numChildren; ++i)
			{
				Main.MissionActions.Actions.List.getChildAt(i).y = lY;
				lY+=33;
			}
			Main.MissionActions.Actions.List.lastY = lY;
			
			MissionList.splice(index, 1);
		}
				
		
		public function showSafelock(isUnlock:Boolean, isShow:Boolean)
		{
			Main.showSafelock(isUnlock, isShow);
			Main.stage.focus = null;
		}
		
		public function enableSafelockInput(enable:Boolean)
		{
			if(enable)
				Main.stage.focus = Main.Safelock.TextNum;
			else
				Main.stage.focus = null;
		}
		
		public function addPlayerToGroup(name:String, isLeader:Boolean, isLeaving:Boolean)
		{
			GroupPlayers.push({name:name, isLeader:isLeader, isLeaving:isLeaving});
			refreshPlayerGroupList();
		}
		public function removePlayerFromGroup(name:String)
		{
			for(var i=0; i<GroupPlayers.length; ++i)
			{
				if(GroupPlayers[i]["name"]==name)
				{
					GroupPlayers.splice(i, 1);
					break;
				}
			}
			refreshPlayerGroupList();
		}
		public function aboutToLeavePlayerFromGroup(name:String)
		{
			for(var i=0; i<GroupPlayers.length; ++i)
			{
				if(GroupPlayers[i]["name"]==name)
				{
					GroupPlayers[i]["isLeaving"] = true;
					break;
				}
			}
			refreshPlayerGroupList();
		}
		
		public function refreshPlayerGroupList()
		{
			if(GroupPlayers.length == 0)
			{
				Main.GroupMenu.visible = false;
				return;
			}
			
			Main.GroupMenu.visible = true;
			for(var i=0; i<10; ++i)
			{
				var Name = "Slot"+(i+1);
				if(i<GroupPlayers.length)
				{
					Main.GroupMenu[Name].visible = true;
					Main.GroupMenu[Name].Name.text = GroupPlayers[i]["name"];
					Main.GroupMenu[Name].NameShad.text = GroupPlayers[i]["name"];
					Main.GroupMenu[Name].Star.visible = GroupPlayers[i]["isLeader"];
					if(GroupPlayers[i]["isLeaving"])
						Main.GroupMenu[Name].Plate.gotoAndPlay("loop");
					else
						Main.GroupMenu[Name].Plate.gotoAndStop(1);
				}
				else
				{
					Main.GroupMenu[Name].visible = false;
				}
			}
		}

		public function addPlayerToVoipList(name:String)
		{
			VOIPPlayers.push({name:name});
			refreshPlayerVoipList();
		}
		public function removePlayerFromVoipList(name:String)
		{
			for(var i=0; i<VOIPPlayers.length; ++i)
			{
				if(VOIPPlayers[i]["name"]==name)
				{
					VOIPPlayers.splice(i, 1);
					break;
				}
			}
			refreshPlayerVoipList();
		}
		
		public function refreshPlayerVoipList()
		{
			while(Main.VOIPList.numChildren > 0)
				Main.VOIPList.removeChildAt(0);
			
			if(VOIPPlayers.length == 0)
			{
				return;
			}
			
			var curY = 0;
			for(var i=0; i<VOIPPlayers.length; ++i)
			{
				var slot = new warz.hud.PlayerVOIPListSlot();
				slot.x = 0;
				slot.y = curY; curY += 25;
				
				slot.NameShad.text = slot.Name.text = VOIPPlayers[i]["name"];

				Main.VOIPList.addChild(slot);
			}
		}

		public function addCharTag(playerName:String, isSameClan:Boolean, reputation:String):MovieClip
		{
			var res = new warz.hud.CharTag();
			res.Name.htmlText = playerName;
			res.NameShad.htmlText = playerName;
			res.Plate.width = res.Name.textWidth + 30;
			res.SameClan.visible = isSameClan;
			res.SameGroup.visible = false;
			//res.VoipIcon.visible = false;
			res.Icons.gotoAndStop(reputation);
			res.Icons.x = res.Plate.x - res.Plate.width/2 - 16;
			Main.UserTagList.addChild(res);
			return res;
		}

		public function removeUserIcon(clip:MovieClip):void
		{
			Main.UserTagList.removeChild(clip);
		}
		
		public function moveUserIcon(clip:MovieClip, posx:Number, posy:Number, isVisible:Boolean)
		{
			clip.x = posx;
			clip.y = posy;
			clip.visible = isVisible;
		}
		public function setCharTagTextVisible(clip:MovieClip, isVisible:Boolean, isSameGroupVisible:Boolean, isVoipVisible:Boolean)
		{
			clip.Name.visible = isVisible;
			clip.NameShad.visible = isVisible;
			clip.Plate.visible = isVisible;
			clip.Icons.visible = isVisible;
			clip.SameGroup.visible = isSameGroupVisible;
			//clip.VoipIcon.visible = isVoipVisible;
		}

		public function showNoteWrite(set:Boolean)
		{
			if(set && !Main.NotesWrite.visible)
			{
				Main.NotesWrite.gotoAndPlay("in");				
				Main.NotesWrite.NotesWrite.Text.text = "";
				Main.NotesWrite.NotesWrite.Text.setSelection(0,0);
				Main.stage.focus = Main.NotesWrite.NotesWrite.Text;
			}
			else if(!set && Main.NotesWrite.visible)
				Main.NotesWrite.gotoAndPlay("out");
				
			Main.NotesWrite.visible = set;
		}
		
		public function showNoteRead(set:Boolean, msg:String)
		{
			if(set && !Main.NotesRead.visible)
			{
				Main.NotesRead.NoteRead.Popup.visible = false;
				Main.NotesRead.NoteRead.Text.text = msg;
				Main.stage.focus = Main.NotesRead.NoteRead.Text;
				Main.NotesRead.gotoAndPlay("in");				
			}
			else if(!set && Main.NotesRead.visible)
				Main.NotesRead.gotoAndPlay("out");
				
			Main.NotesRead.visible = set;
		}

		public function showGraveNote(set:Boolean, msg:String, player1:String, player2:String)
		{
			if(set && !Main.Grave.visible)
			{
				Main.Grave.Title.text = msg;
				Main.Grave.Player1.text = player1;
				Main.Grave.Player2.text = player2;
			}
				
			Main.Grave.visible = set;
		}

		public function clearPlayersList()
		{
			Players = new Array();
		}
		public function addPlayerToList(index:int, pos:int, Name:String, alignment:String, isLegend:Boolean, isDev:Boolean, isPunisher:Boolean, isInvitePending:Boolean, isVoipMuted:Boolean, isPremium:Boolean, isLocalPlayer:Boolean)
		{
			Players.push({index:index, pos:pos, Name:Name, alignment:alignment, isLegend:isLegend, isDev:isDev, isPunisher:isPunisher, isInvitePending:isInvitePending, isVoipMuted:isVoipMuted, isPremium:isPremium, isLocalPlayer:isLocalPlayer, movie:null});
		}
		public function setPlayerInListVoipVisible(index:int, vis:Boolean, muted:Boolean)
		{
			for(var i=0; i<Players.length; ++i)
			{
				if(Players[i]["index"]==index)
				{
					Players[i]["movie"].VoipIcon.visible = vis;
					Players[i]["movie"].VoipIconMuted.visible = muted; 
					break;
				}
			}
		}
		public function showPlayersList(flag:Boolean)
		{
			Main.PlayerList_Dropmenu.visible = false;

			if(flag)
			{
				// delete old slots
				while(Main.PlayersList.List.numChildren > 0)
					Main.PlayersList.List.removeChildAt(0);
				var startX = 51.45;
				var startY = 24.5;
				var offsetX = 503.45-startX;
				var offsetY = 67-startY;
				
				var curX = startX;
				var curY = startY;
				
				var intX = 0;
				
				for(var i=0; i<Players.length; ++i)
				{
					var slot = new warz.hud.PlayersListSlot();
					slot.x = curX;
					slot.y = curY;
					
					Players[i]["movie"] = slot;
					
					Main.PlayersList.List.addChild(slot);
					
					slot.plrIndex = Players[i]["index"];
					slot.NumShad.text = slot.Num.text = Players[i]["pos"];
					slot.NameShad.text = slot.Name.text = Players[i]["Name"];
					slot.AlignmentShad.text = slot.Alignment.text = Players[i]["alignment"];
					slot.IconLegend.visible = Players[i]["isLegend"];
					slot.IconPunisher.visible = Players[i]["isPunisher"];
					slot.isDevIcon.visible = Players[i]["isDev"];
					slot.VoipIcon.visible = false;
					slot.VoipIconMuted.visible = Players[i]["isVoipMuted"];
					slot.IconPremium.visible = Players[i]["isPremium"];
					slot.PlateInviteStatic.visible = slot.PlateInvite.visible = Players[i]["isInvitePending"];
					slot.PlatePlayer.visible = Players[i]["isLocalPlayer"];
					slot.gotoAndStop(1);
					
					slot.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event) 
										  {
										  	evt.currentTarget.parent.gotoAndPlay("over");
										  });
					slot.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event) 
										  {
										  	evt.currentTarget.parent.gotoAndPlay("out");
										  });
					slot.Btn.addEventListener(MouseEvent.CLICK, function(evt:MouseEvent) 
										  {
  									  		  if (evt is MouseEventEx)
											  	if ((evt as MouseEventEx).buttonIdx == MouseEventEx.RIGHT_BUTTON)
											  	{
												  SoundEvents.eventSoundPlay("menu_click");
												  evt.currentTarget.parent.gotoAndPlay("pressed");
												  Main.PlayerList_Dropmenu.visible = false;
												}
										  });
					slot.Btn.addEventListener(MouseEvent.CLICK, function(evt:MouseEvent) 
										  {
												  SoundEvents.eventSoundPlay("menu_click");
												  evt.currentTarget.parent.gotoAndPlay("pressed");
												  PlayerList_CurrentPlayerSelected = evt.currentTarget.parent.Name.text;
												  var p:Point = evt.currentTarget.parent.localToGlobal(new Point(evt.localX, evt.localY));
												  Main.PlayerList_Dropmenu.x = p.x + Main.PlayerList_Dropmenu.width/2;
												  Main.PlayerList_Dropmenu.y = p.y;// - Main.PlayerList_Dropmenu.height/2;
												  HUDEvents.eventShowPlayerListContextMenu(PlayerList_CurrentPlayerSelected);
												  if(api.isDebug)
												  {
													  setPlayerListContextMenuButton(1, "$HUD_PlayerAction_Report", 1);
													  setPlayerListContextMenuButton(2, "", 0);
													  setPlayerListContextMenuButton(3, "", 0);
													  setPlayerListContextMenuButton(4, "$HUD_PlayerAction_Kick", 4);
													  setPlayerListContextMenuButton(5, "$HUD_PlayerAction_Ban", 5);
													  setPlayerListContextMenuButton(6, "", 0);
												  	  showPlayerListContextMenu();
												  }
										  });
					
					curX += offsetX;
					intX += 1;
					if(intX >= 4)
					{
						curX = startX;
						intX = 0;
						curY += offsetY;
					}
				}
				
				Main.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
				Main.addEventListener(MouseEvent.MOUSE_UP, endDrag);
				
				Main.PlayersList.List.y=205;
				
				if (Players.length > 100)
				{
					Scroller.visible = true;
					var me = this;
					
					Scroller.Trigger.y = Scroller.Field.y;
					Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
					
					Scroller.ArrowDown.y = Main.PlayersList.Mask.height - 4;
					Scroller.Field.height = (Main.PlayersList.Mask.height - (Scroller.ArrowDown.height * 2));
				}
				else
				{
					Scroller.visible = false;
					Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				}
			}
			else
			{
				Main.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
				Main.removeEventListener(MouseEvent.MOUSE_UP, endDrag);
			}
			Main.PlayersList.visible = flag;
		}
		
		public function endDrag(e:MouseEvent) 
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
		}

		public	function scrollItems(e:Event)
		{
			if (!Scroller || Scroller.visible == false)
			{
				Main.PlayersList.List.y = 205;
				return;
			}
			
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = Main.PlayersList.List.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = Main.PlayersList.Mask.height;
			h -= mh;
			h /= h1;
			
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 42);
			
			if (Main.PlayersList.List.y != 42 * step)
			{
				Tweener.addTween(Main.PlayersList.List, {y:205+42*step, time:0.2, transition:"linear"});
			}
		}
		
		public function setPlayerListContextMenuButton(id:int, name:String, actionID:int)
		{
			var Name = "Btn"+id;
			
			Main.PlayerList_Dropmenu[Name].Text.Text.text = name;
			Main.PlayerList_Dropmenu[Name].visible = (name.length>0);
			Main.PlayerList_Dropmenu[Name].actionID = actionID;
		}
		public function showPlayerListContextMenu()
		{
			Main.PlayerList_Dropmenu.visible = true;
		}
		
		public function setThreatValue(percent:int)
		{
			percent = Math.max (0, percent);
			percent = Math.min (percent, 100);

			Main.ThreatIndicator.gotoAndStop(percent);
		}
		
		/**
		 * setWeaponInfo
		 *
		 * Ammo
		 * clips
		 * type - Possible values "three", "one", "auto"
		 **/
		public	function setWeaponInfo (ammo:int, clips:int, type:String, durability:int):void
		{
			Main.Weapon.Weapon.Ammo.text = ammo;
			Main.Weapon.Weapon.AmmoShad.text = ammo;
			Main.Weapon.Weapon.Clips.text = clips;
			Main.Weapon.Weapon.ClipsShad.text = clips;
			Main.Weapon.Weapon.DuraScale.gotoAndStop(Math.max(1, Math.min(100, 100-durability)));
			
			if (Main.Weapon.Weapon.Fire.currentLabel != type)
				Main.Weapon.Weapon.Fire.gotoAndPlay(type);
		}
		
		/**
		 * showWeaponInfo
		 *
		 **/
		public	function showWeaponInfo (state:Boolean):void
		{
			if (state)
			{
				if (Main.Weapon.currentLabel != "start")
					Main.Weapon.gotoAndPlay ("start");
			}
			else
			{
				if (Main.Weapon.currentLabel != "out")
					Main.Weapon.gotoAndPlay ("out");
			}
		}
		
		public function setConditionIconVisibility(icon:String, visibl:Boolean)
		{
			Main.Condition[icon].gotoAndStop(visibl?"on":"off");
		}
		
		/**
		 * setHeroCondition
		 *
		 * food - 0 - 200: 0 = Empty, 100 = Full, 200 = Empty
		 * water - 0 - 200: 0 = Empty, 100 = Full, 200 = Empty
		 * health - 0 - 200: 0 = Full, 100 = Empty, 200 = Full
		 * toxicity - 0 - 200: 0 = Empty, 100 = Full, 200 = Empty
		 * stamina - 0 - 100: 0 = Empty, 100 = Full
		 **/
		public function setHeroCondition (food:int, water:int, health:int, toxicity:int, stamina:int)
		{
			Main.Condition.Food.gotoAndStop (food);
			Main.Condition.Water.gotoAndStop (water);
			Main.Condition.Character.gotoAndStop (health);
			Main.Condition.Stamina.gotoAndStop (stamina);
			Main.Condition.Character.Character.gotoAndStop (toxicity);
			
			Main.Condition.ToxicityIcon.visible = toxicity > 0;
			Main.Condition.ToxicityIcon.Text.Text.text = toxicity+"%";
			Main.Condition.ToxicityIcon.Text.TextShad.text = toxicity+"%";

			if (Main.Condition.Stamina.currentFrame != 100)
				Main.Condition.Stamina.visible = true;
			else
				Main.Condition.Stamina.visible = false;
		}
		
		public function setSlotCooldown(slotID:int, progress:int, timeLeft:int)
		{
			var	name = "Slot" + String (slotID + 1);
			Main.Slots.Slots[name].CooldownBar.TextShad.Text.text=Main.Slots.Slots[name].CooldownBar.Text.Text.text=String(timeLeft)+" $FR_Cooldown_TimeInSeconds";
			if(progress<100)
				Main.Slots.Slots[name].CooldownBar.gotoAndStop(progress);
			else if(progress==100)
				Main.Slots.Slots[name].CooldownBar.gotoAndPlay(progress);
			else
				Main.Slots.Slots[name].CooldownBar.gotoAndStop(107);			
		}
		
		public	function setSlot (slotID:int, itemName:String, quantity:int, icon:String):void
		{
			var	name = "Slot" + String (slotID + 1);
			
			Main.Slots.Slots[name].visible = true;
			Main.Slots.Slots[name].PlateText.Num.text = String (slotID + 1);
			Main.Slots.Slots[name].PlateTextShad.Num.text = String (slotID + 1);
			Main.Slots.Slots[name].PlateText.Name.text = itemName;
			Main.Slots.Slots[name].PlateTextShad.Name.text = itemName;
			
			if (quantity > 0)
			{
				Main.Slots.Slots[name].Text.Text.text = String (quantity);
				Main.Slots.Slots[name].TextShad.Text.text = String (quantity);
			}
			else
			{
				Main.Slots.Slots[name].Text.Text.text = "";
				Main.Slots.Slots[name].TextShad.Text.text = "";
			}

			if(Main.Slots.Slots[name].prevPicIcon != icon)
			{
				while (Main.Slots.Slots[name].Pic.numChildren > 0)
					Main.Slots.Slots[name].Pic.removeChildAt(0);
				
				if (icon != "")
					loadSlotIcon (icon, Main.Slots.Slots[name].Pic);
				
				Main.Slots.Slots[name].prevPicIcon = icon;
			}

//			Main.Slots.Slots[name].gotoAndStop(0);
		}

		/**
		 * setActiveSlot
		 *
		 * slotID - true / false
		 **/
		public	function setActiveSlot (slotID:int):void
		{
			for (var a:int = 0; a < 6; a++)
			{
				if (a != slotID)
				{
					var name = "Slot" + String (a + 1);
					
					if (Main.Slots.Slots[name].currentLabel == "pressed_out")
						Main.Slots.Slots[name].gotoAndStop(0);
				}
			}
				
			name = "Slot" + String (slotID + 1);
			if(Main.Slots.Slots[name] != undefined)
				if (Main.Slots.Slots[name].currentLabel != "pressed_out")
				{
					Main.Slots.Slots[name].gotoAndPlay ("pressed_out");
					SoundEvents.eventSoundPlay("weapon_quick_change");
				}
		}
		
		/**
		 * setActivatedSlot
		 *
		 * slotID
		 **/
		public	function setActivatedSlot (slotID:int):void
		{
			for (var a:int = 0; a < 6; a++)
			{
				if (a != slotID)
				{
					var name = "Slot" + String (a + 1);
					
					if (Main.Slots.Slots[name].currentFrame == 20)
						Main.Slots.Slots[name].gotoAndPlay(0);
				}
			}
			
			name = "Slot" + String (slotID + 1);
			
			if (Main.Slots.Slots[name].currentLabel != "activate_old")
				Main.Slots.Slots[name].gotoAndPlay ("activate_old");
		}
		
		/**
		 * updateSlot
		 *
		 * slotID
		 * quantity
		 **/
		public	function updateSlot (slotID:int, quantity:int):void
		{
			var	name = "Slot" + String (slotID + 1);
			Main.Slots.Slots[name].visible = true;
			Main.Slots.Slots[name].Text.Text.text = String (quantity);
			Main.Slots.Slots[name].TextShad.Text.text = String (quantity);
		}
		
		/**
		 * showSlots
		 *
		 * show - true / false
		 **/
		public	function showSlots (show:Boolean)
		{
			if (show)
			{
				if (Main.Slots.currentLabel != "start")
					Main.Slots.gotoAndPlay ("start");
			}
			else
			{
				if (Main.Slots.currentLabel != "out")
					Main.Slots.gotoAndPlay ("out");
			}
		}
		
		/**
		 * showMsg
		 *
		 * text
		 **/
		public	function showMsg (text:String):void
		{
			var	msg = new warz.hud.msg ();
			
			msg.Msg.Text.text = text;
			msg.Msg.TextShad.text = text;
			msg.gotoAndPlay (0);
			
			Main.Msg.addChild(msg);
			
			setTimeout(killMsg, 3000, msg);
		}
		
		/**
		 * showChat
		 *
		 * show - true / false
		 **/
		public	function showChat (showChat:Boolean, showInput:Boolean = true, inputText:String = ""):void
		{
			this.showchat = showChat;
			this.showInput = showInput;
			
			if (!showChat && !showInput)
			{
//				if (Main.Chat.currentLabel != "out")
//					Main.Chat.gotoAndPlay ("out");
					
				Main.Chat.Chat.InputText.removeEventListener(KeyboardEvent.KEY_DOWN, keyboardInput);
				Main.Chat.Chat.InputText.removeEventListener(Event.ENTER_FRAME, chatInputEnterFrame);
				Main.stage.focus = null;
			}
			else
			{
				var	chat = Main.Chat;
				var	me = this;
				
				if (showChat)
					chat.Chat.Plate.visible = true;
				else
					chat.Chat.Plate.visible = false;
				
//				if (Main.Chat.currentLabel != "start")
//					Main.Chat.gotoAndPlay ("start");
					
				if (showInput)
				{
					chat.Chat.TabPlates.visible = true;
					if (chat.Chat.Tab1.Active)
						chat.Chat.Tab1.visible = true;
					if (chat.Chat.Tab2.Active)
						chat.Chat.Tab2.visible = true;
					if (chat.Chat.Tab3.Active)
						chat.Chat.Tab3.visible = true;
					if (chat.Chat.Tab4.Active)
						chat.Chat.Tab4.visible = true;
					chat.Chat.InputText.visible = true;
					chat.Chat.InputTextBack.visible = true;

					Main.Chat.Chat.InputText.addEventListener(KeyboardEvent.KEY_DOWN, keyboardInput)
				}
				else
				{
					chat.Chat.TabPlates.visible = false;
					chat.Chat.Tab1.visible = false;
					chat.Chat.Tab2.visible = false;
					chat.Chat.Tab3.visible = false;
					chat.Chat.Tab4.visible = false;
					chat.Chat.InputText.visible = false;
					chat.Chat.InputTextBack.visible = false;
					Main.Chat.Chat.InputText.removeEventListener(KeyboardEvent.KEY_DOWN, keyboardInput)
					Main.Chat.Chat.InputText.removeEventListener(Event.ENTER_FRAME, chatInputEnterFrame);
					Main.stage.focus = null;
				}
				
				Main.Chat.Chat.InputText.text = inputText;
			}
		}
		
		/**
		 * setChatTab
		 *
		 * id
		 * name
		 * active - true / false
		 * visible - true / false
		 **/
		public	function setChatTab (id:int, name:String, active:Boolean, visible:Boolean):void
		{
			var	tabName = "Tab" + String (id + 1);
			var	tabPlateName = "TabPlates" + String (id + 1);
			var	me = this;
			
			Main.Chat.Chat[tabName].Text.Text.text = name;
			Main.Chat.Chat[tabName].Text.TextShad.text = name;
			Main.Chat.Chat[tabName].Tab = id;
			Main.Chat.Chat[tabName].visible = visible;
			Main.Chat.Chat[tabName].Active = visible;
			
			if (Main.Chat.Chat.TabPlates.currentLabel != tabName)
				Main.Chat.Chat.TabPlates.gotoAndPlay (tabName);

			Main.Chat.Chat[tabName].Active = active;
			
			if (active)
			{
				if (Main.Chat.Chat[tabName].currentLabel != "over")
					Main.Chat.Chat[tabName].gotoAndPlay ("over");
					
				activeChatTab = id;
			}
			else
			{
				if (Main.Chat.Chat[tabName].currentLabel != "out")
					Main.Chat.Chat[tabName].gotoAndPlay ("out");
			}
		}
		
		/**
		 * setChatTabActive
		 *
		 * id
		 **/
		public	function setChatTabActive (id:int):void
		{
			if (this.showInput)
			{
				for (var a:int = 0; a < 4; a++)
				{
					var	tabName = "Tab" + String (a + 1);
					
					if (Main.Chat.Chat[tabName].Active)
						Main.Chat.Chat[tabName].gotoAndPlay("out");
	
					Main.Chat.Chat[tabName].Active = false;
				}
				
				tabName = "Tab" + String (id + 1);
				Main.Chat.Chat[tabName].gotoAndPlay("over");
				Main.Chat.Chat[tabName].Active = true;
	
				if (Main.Chat.Chat.TabPlates.currentLabel != tabName)
					Main.Chat.Chat.TabPlates.gotoAndPlay (tabName);
				
				activeChatTab = id;
			}
		}
		
		/**
		 * setChatActive
		 *
		 **/
		public	function setChatActive ():void
		{
			Main.stage.focus = Main.Chat.Chat.InputText;
			Main.Chat.Chat.InputText.addEventListener(Event.ENTER_FRAME, chatInputEnterFrame)
		}
		
		/**
		 * setChatActive
		 *
		 * alpha - 0 - 1.0
		 **/
		public	function setChatTransparency (alpha:Number):void
		{
			Main.Chat.Chat.TabPlates.alpha = alpha;
			Main.Chat.Chat.Plate.alpha = alpha
		}
		
		/**
		 * receiveChat
		 *
		 * tab
		 * user
		 * text
		 **/
		public	function receiveChat (text:String):void
		{
			var	chat = new warz.hud.ChatTextBlock ();
			var clip = Main.Chat.TextWindows[0];
			
			//TextFieldEx.setNoTranslate(chat.Text, true);
			chat.ChatOutputText.htmlText = text;
			
			var	texHeight = chat.ChatOutputText.height;
			var	height = chat.ChatOutputText.textHeight;
			
			chat.ChatOutputText.height = chat.ChatOutputText.textHeight + 4;
//			chat.TextShad.height = chat.TextShad.textHeight + 4;
			
			clip.addChild(chat);
			clip.Chats.push (chat);
			
			height = 300;
			for (var a:int = clip.Chats.length - 1; a >= 0; a--)
			{
				chat = clip.Chats[a];
				chat.y = height - chat.height;
				
				height -= chat.height;
			}
			
			while (clip.Chats[0] && clip.Chats[0].y + clip.Chats[0].height < 0)
			{
				clip.removeChild(clip.Chats[0]);
				clip.Chats.splice (0, 1);
			}
			
			Main.Chat.TextWindows[0].visible = true;
			Main.Chat.TextWindows[0].mask = Main.Chat.Chat.ChatMasker;//cynthia: 1778
		}

/**********************************************************************
 *
 * Healper Functions
 * 
 **********************************************************************/
		public function loadSlotIcon(path:String, imageHolder:MovieClip)
		{
			var	dat:Object = {imageHolder:imageHolder};
			var	imageLoader:ImageLoader = new ImageLoader (path, loadSlotCallback, dat)
		}
		
		public	function loadSlotCallback (bitmap:Bitmap, dat:Object)
		{
			bitmap = Layout.stretch (bitmap, 189, 117, bitmap.width, bitmap.height, "uniform");
			bitmap.x = -37;
			bitmap.y = 0;
			
			dat.imageHolder.addChild (bitmap);
		}

		public	function killMsg (msg:Object)
		{
			Main.Msg.removeChild(msg);
		}
		
		public	function startDebugTimer ()
		{
			Main.startDebugTimer ();
		}
		
		public	function keyboardInput(evt:KeyboardEvent):void
		{ 
			if (evt.keyCode == 13)
			{
// send message					
				HUDEvents.eventChatMessage(activeChatTab, Main.Chat.Chat.InputText.text);
				Main.Chat.Chat.InputText.text = "";
				Main.Chat.Chat.InputText.removeEventListener(Event.ENTER_FRAME, chatInputEnterFrame);
				Main.stage.focus = null;
			}
			else if (evt.keyCode == 27)
			{
				HUDEvents.eventChatMessage(-1, "");
				Main.Chat.Chat.InputText.text = "";
				Main.Chat.Chat.InputText.removeEventListener(Event.ENTER_FRAME, chatInputEnterFrame);
				Main.stage.focus = null;
			}
		};
		
		public function chatInputEnterFrame(evt:Event):void
		{
			if(Main.stage.focus != Main.Chat.Chat.InputText)
			{
				HUDEvents.eventChatMessage(activeChatTab, "");
				Main.Chat.Chat.InputText.text = "";
				Main.Chat.Chat.InputText.removeEventListener(KeyboardEvent.KEY_DOWN, keyboardInput);
				
				Main.Chat.Chat.InputText.removeEventListener(Event.ENTER_FRAME, chatInputEnterFrame);
			}
		}
	}
}