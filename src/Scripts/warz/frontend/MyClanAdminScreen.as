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

	public class MyClanAdminScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var ClanApps:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;

		private var currentSort = "name";
		
		private var clanApplications:Array = new Array();
		private var currentAppID:uint = 0;
		private var currentselectedSlotID:int = 0;

		public	function MyClanAdminScreen ()
		{
			visible = false;
			var me = this;
			
			Scroller = ClanApps.Scroller;
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
			
			ClanApps.Tab1.Text.Text.text = "$FR_CLANS";
			ClanApps.Tab2.Text.Text.text = "$FR_MYCLAN";
			ClanApps.Tab3.Text.Text.text = "$FR_MYCLANAPPLICATIONS";
			
			ClanApps.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			ClanApps.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			ClanApps.Tab1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			ClanApps.Tab1.ActionFunction = function() {api.Main.showScreen("Clans");}

			ClanApps.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			ClanApps.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			ClanApps.Tab2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			ClanApps.Tab2.ActionFunction = function()  {api.Main.showScreen("MyClan");}

			ClanApps.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			ClanApps.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			ClanApps.Tab3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			ClanApps.Tab3.ActionFunction = function() {}

			ClanApps.MenuBtnLeader.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ClanApps.MenuBtnLeader.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ClanApps.MenuBtnLeader.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			ClanApps.MenuBtnLeader.ActionFunction = function() {api.Main.showScreen("Leaderboard");}
			ClanApps.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ClanApps.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ClanApps.MenuBtnFriends.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			ClanApps.MenuBtnFriends.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}
			ClanApps.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ClanApps.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ClanApps.MenuBtnNews.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			ClanApps.MenuBtnNews.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}
			
			ClanApps.MenuBtnFriends.visible = false;
			ClanApps.MenuBtnNews.visible = false;
			
			ClanApps.BtnInvite.Text.Text.text = "$FR_ClanInvitePlayer";
			ClanApps.BtnInvite.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ClanApps.BtnInvite.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ClanApps.BtnInvite.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			ClanApps.BtnInvite.ActionFunction = function() 
			{
				api.Main.MsgBox.showInfoInputMsg("$FR_ClanInvitePlayerText", "$FR_ClanInvitePlayer", inviteToClanCallback);
			}

			ClanApps.DescrBlock.BtnDecline.Text.Text.text = "$FR_DECLINE";
			ClanApps.DescrBlock.BtnDecline.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ClanApps.DescrBlock.BtnDecline.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ClanApps.DescrBlock.BtnDecline.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			ClanApps.DescrBlock.BtnDecline.ActionFunction = function() 
			{
				FrontEndEvents.eventClanApplicationAction(currentAppID, false);
				clanApplications.splice(currentselectedSlotID, 1);
				showApplications();				
			}

			ClanApps.DescrBlock.BtnAccept.Text.Text.text = "$FR_ACCEPT";
			ClanApps.DescrBlock.BtnAccept.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ClanApps.DescrBlock.BtnAccept.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ClanApps.DescrBlock.BtnAccept.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			ClanApps.DescrBlock.BtnAccept.ActionFunction = function() 
			{
				FrontEndEvents.eventClanApplicationAction(currentAppID, true);
				clanApplications.splice(currentselectedSlotID, 1);
				showApplications();				
			}

			ClanApps.ListTop.Btn2.Text.Text.text = "$FR_ClanPlName";
			ClanApps.ListTop.Btn3.Text.Text.text = "$FR_ClanPlExp";
			ClanApps.ListTop.Btn4.Text.Text.text = "$FR_ClanPlST";
			ClanApps.ListTop.Btn5.Text.Text.text = "$FR_ClanPlRep";
			ClanApps.ListTop.Btn6.Text.Text.text = "$FR_ClanPlKZ";
			ClanApps.ListTop.Btn7.Text.Text.text = "$FR_ClanPlKS";
			ClanApps.ListTop.Btn8.Text.Text.text = "$FR_ClanPlKB";
			//ClanApps.ListTop.Btn9.Text.Text.text = "$FR_ClanPlDG";
		}
		
		public	function inviteToClanCallback (state:Boolean, text:String=""):void
		{
			if (state)
			{
				FrontEndEvents.eventClanInviteToClan(text);
			}
		}

		
		public function BtnRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
		}
		public function BtnRollOutFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		public function BtnPressFn(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
			evt.currentTarget.parent.gotoAndPlay("pressed");
			evt.currentTarget.parent.ActionFunction();
		}

		public function BtnRollOverFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		public function BtnRollOutFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		public function BtnPressFn2(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction();
			}		
		}

		/*public function BtnRollOverFn3(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		public function BtnRollOutFn3(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		public function BtnPressFn3(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				for(var i =1; i<4; ++i)
				{
					var n = "Sorting"+i;
					if(Clans.Sorting[n].State == "active")
					{
						Clans.Sorting[n].State = "none";
						Clans.Sorting[n].gotoAndPlay("out");
					}
				}
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				currentSort = evt.currentTarget.parent.sortName;
				populateClanList();
			}
		}*/

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			
			currentSort = "name";

			ClanApps.Tab3.gotoAndPlay("pressed");
			ClanApps.Tab3.State = "active";
			ClanApps.MenuBtnClans.gotoAndPlay("pressed");
			
			FrontEndEvents.eventRequestClanApplications();
			clanApplications = new Array();
			if(api.isDebug)
			{
				addApplication(0, "dsf897 ksdfysdif kdsfhsdkjf sdkjfhdf", "first", 3434, "12:12:1212", "bulshit", 2323, 35345, 546);
				for(var i=0; i<50; ++i)
					addApplication(i, "dskjfhsdkjfhdsfkjh", "blabla", 3434, "12:12:1212", "bulshit", 2323, 35345, 546);
				addApplication(100,"ds87dfdsj skjdfysidf7", "last", 3434, "12:12:1212", "bulshit", 2323, 35345, 546);
				showApplications();
			}

			this.visible = true;
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
		}
		
		public function Deactivate()
		{
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);
			this.visible = false;
		}
		
		public	function mouseUp(e:Event) :void
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
		}

		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > ClanApps.Mask.x && 
				e.stageX < ClanApps.Mask.x + ClanApps.Mask.width + 45 &&
				e.stageY > ClanApps.Mask.y && 
				e.stageY < ClanApps.Mask.y + ClanApps.Mask.height)
			{
				var dist = (ClanApps.Slots.height - ClanApps.Mask.height) / 48;
				var h = Scroller.Field.height - Scroller.Trigger.height + 26;
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
				var endY = Scroller.Field.height - Scroller.Trigger.height+26;
				if(Scroller.Trigger.y < endY) { 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y > endY) 
						Scroller.Trigger.y = endY; 
				} 
			}
		}

		private function sortByName(a, b) :Number
		{
			var n1 = a["name"];
			var n2 = b["name"];
			
			if(n1 > n2) return 1;
			else if(n1 < n2) return -1;
			else return 0;			
		}
		private function sortByXP(a, b) :Number
		{
			var n1 = a["xp"];
			var n2 = b["xp"];
			
			if(n1 > n2) return 1;
			else if(n1 < n2) return -1;
			else return 0;			
		}
		private function sortByMembers(a, b) :Number
		{
			var n1 = a["numMembers"];
			var n2 = b["numMembers"];
			
			if(n1 > n2) return 1;
			else if(n1 < n2) return -1;
			else return 0;			
		}		

		public function addApplication(appID:uint, appText:String, name:String, exp:uint, stime:String, rep:String, kz:uint, ks:uint, kb:uint)
		{
			clanApplications.push({appID:appID, appText:appText, name:name, exp:exp, stime:stime, rep:rep, kz:kz, ks:ks, kb:kb, movie:null});
		}
		
		public function showApplications()
		{
			ClanApps.DescrBlock.gotoAndStop("inactive");
			ClanApps.DescrBlock.Text.text = "";
			while (ClanApps.Slots.numChildren > 0)
			{
				ClanApps.Slots.removeChildAt(0);
			}			
			
			/*if(currentSort == "name")
			{
				clansInfo.sort(sortByName);
			}
			else if(currentSort == "xp")
			{
				clansInfo.sort(sortByXP);
			}
			else if(currentSort == "members")
			{
				clansInfo.sort(sortByMembers);
			}*/
			
			
			var y = 0;
			for(var i=0; i<clanApplications.length; ++i)
			{
				var slot = new warz.frontend.ClansMyClanSlot();
				clanApplications[i]["movie"] = slot;
				slot.appInfo = clanApplications[i];
				slot.appID = i;
				slot.x = 0;
				slot.y = y;
				slot.MemberIcons.visible = false;
				slot.Text.P.text = i+1;
				slot.Text.Name.text = clanApplications[i]["name"];
				slot.Text.Exp.text = clanApplications[i]["exp"];
				slot.Text.Time.text = clanApplications[i]["stime"];
				slot.Text.Reputation.text = clanApplications[i]["rep"];
				slot.Text.KilledZombies.text = clanApplications[i]["kz"];
				slot.Text.KilledSurvivors.text = clanApplications[i]["ks"];
				slot.Text.KilledBandits.text = clanApplications[i]["kb"];
				//slot.Text.DonatedGC.text = "N/A";
				
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
												  for(var j=0; j<clanApplications.length; ++j)
												  {
													  if(clanApplications[j]["movie"].State == "active")
													  {
														  clanApplications[j]["movie"].State = "none";
														  clanApplications[j]["movie"].gotoAndPlay("out");
													  }
												  }
												  SoundEvents.eventSoundPlay("menu_click");
												  evt.currentTarget.parent.State = "active";
												  evt.currentTarget.parent.gotoAndPlay("pressed");
												  evt.currentTarget.parent.ActionFunction();
											  }
										  });
				slot.ActionFunction = function()
				{
					currentselectedSlotID = this.appID;
					currentAppID = this.appInfo["appID"];
					ClanApps.DescrBlock.gotoAndStop("active");
					ClanApps.DescrBlock.Text.text = this.appInfo["appText"];
				}
				
				ClanApps.Slots.addChild(slot);
				
				y+=48;
			}
			
			if(clanApplications.length > 7)
			{
				Scroller.visible = true;
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = ClanApps.Mask.height - 4;
				Scroller.Field.height = (ClanApps.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				ClanApps.Slots.y = 333;
			}				
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = ClanApps.Slots.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = ClanApps.Mask.height;
			h -= mh;
			h /= h1;

			var	dest:Number = dist * h;
			var	step:Number = int (dest / 48);
			
			if (ClanApps.Slots.y != (333+48*step))
			{
				Tweener.addTween(ClanApps.Slots, {y:333+(48*step), time:api.tweenDelay, transition:"linear"});
			}
		}
		
		
		public function loadSlotIcon(path:String, imageHolder:MovieClip, slotType:Number)
		{
			var	dat:Object = {imageHolder:imageHolder, slotType:slotType};
			var	imageLoader:ImageLoader = new ImageLoader (path, loadSlotCallback, dat)
		}
		
		public	function loadSlotCallback (bitmap:Bitmap, dat:Object)
		{
			var slotWidth=0, slotHeight=0;
			var slotX=0, slotY=0;
			
			slotWidth = 325;
			slotHeight = 237;
			slotX = 1.9;
			slotY = 4.55;
			
			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = slotX; 
			bitmap.y = slotY; 
		
			dat.imageHolder.addChild (bitmap);
		}
	}
}