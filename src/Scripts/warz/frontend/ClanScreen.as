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

	public class ClanScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var Clans:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;

		private var clansInfo:Array = new Array();

		private var currentSort:int = 1;
		private var selectedClanID:uint = 0;

		public	function ClanScreen ()
		{
			visible = false;
			var me = this;
			
			Scroller = Clans.Scroller;
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
			
			Clans.Tab1.Text.Text.text = "$FR_CLANS";
			Clans.Tab2.Text.Text.text = "$FR_MYCLAN";
			Clans.Tab3.Text.Text.text = "$FR_MYCLANAPPLICATIONS";
			
			Clans.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			Clans.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			Clans.Tab1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			Clans.Tab1.ActionFunction = function() {}

			Clans.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			Clans.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			Clans.Tab2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			Clans.Tab2.ActionFunction = function() 
			{
				if(api.myClanInfo.myClanID == 0)
					api.Main.showScreen("CreateClan");
				else
					api.Main.showScreen("MyClan");
			}

			Clans.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			Clans.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			Clans.Tab3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			Clans.Tab3.ActionFunction = function() {api.Main.showScreen("MyClanAdmin");}

			Clans.MenuBtnLeader.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.MenuBtnLeader.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.MenuBtnLeader.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.MenuBtnLeader.ActionFunction = function() {api.Main.showScreen("Leaderboard");}
			Clans.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.MenuBtnFriends.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.MenuBtnFriends.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}
			Clans.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.MenuBtnNews.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.MenuBtnNews.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}

			Clans.MenuBtnFriends.visible = false;
			Clans.MenuBtnNews.visible = false;

			Clans.BtnJoin.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.BtnJoin.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.BtnJoin.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.BtnJoin.ActionFunction = function() 
			{
				Clans.JoinClanPopUp.visible = true;
				Clans.JoinClanPopUp.Text.text = "";
			}
			
			Clans.BtnSearchClan.Text.Text.text = "$FR_ClanSearch";
			Clans.BtnSearchClan.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.BtnSearchClan.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.BtnSearchClan.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.BtnSearchClan.visible = false;
			Clans.BtnSearchClan.ActionFunction = function() 
			{
				
			}

			Clans.BtnNavLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.BtnNavLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.BtnNavLeft.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.BtnNavLeft.ActionFunction = function() 
			{
				FrontEndEvents.eventRequestClanList(currentSort, 1);
			}

			Clans.BtnNavTop.Text.Text.text = "$FR_Top";
			Clans.BtnNavTop.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.BtnNavTop.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.BtnNavTop.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.BtnNavTop.ActionFunction = function() 
			{
				FrontEndEvents.eventRequestClanList(currentSort, 0);
			}

			Clans.BtnNavRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.BtnNavRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.BtnNavRight.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.BtnNavRight.ActionFunction = function() 
			{
				FrontEndEvents.eventRequestClanList(currentSort, 2);
			}

			
			Clans.Sorting.Sorting1.Text.Text.text = "$FR_CLANNAME";
			Clans.Sorting.Sorting1.sortID = 1;
			Clans.Sorting.Sorting2.Text.Text.text = "$FR_CLANMEMBERS";
			Clans.Sorting.Sorting2.sortID = 2;
			
			Clans.Sorting.Sorting1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Clans.Sorting.Sorting1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Clans.Sorting.Sorting1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			Clans.Sorting.Sorting2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			Clans.Sorting.Sorting2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			Clans.Sorting.Sorting2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			
			Clans.JoinClanPopUp.CancelBtn.Text.Text.text = "$FR_Cancel";
			Clans.JoinClanPopUp.ApplyBtn.Text.Text.text = "$FR_Apply";
			Clans.JoinClanPopUp.CancelBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.JoinClanPopUp.CancelBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.JoinClanPopUp.CancelBtn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.JoinClanPopUp.CancelBtn.ActionFunction = function() {Clans.JoinClanPopUp.visible = false;}
			Clans.JoinClanPopUp.ApplyBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Clans.JoinClanPopUp.ApplyBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Clans.JoinClanPopUp.ApplyBtn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			Clans.JoinClanPopUp.ApplyBtn.ActionFunction = function() 
			{
				Clans.JoinClanPopUp.visible = false;
				FrontEndEvents.eventClanApplyToJoin(selectedClanID, Clans.JoinClanPopUp.Text.text);
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

		public function BtnRollOverFn3(evt:Event)
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
				for(var i =1; i<3; ++i)
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
				
				currentSort = evt.currentTarget.parent.sortID;
				FrontEndEvents.eventRequestClanList(currentSort, 0);
			}
		}

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			
			currentSort = 1;

			Clans.JoinClanPopUp.visible = false;
			Clans.Tab1.gotoAndPlay("pressed");
			Clans.Tab1.State = "active";
			Clans.MenuBtnClans.gotoAndPlay("pressed");
			Clans.Sorting.Sorting1.gotoAndPlay("pressed");
			Clans.Sorting.Sorting1.State = "active";
			
			this.visible = true;
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);

			FrontEndEvents.eventRequestMyClanInfo();
			if(api.isDebug)
			{
				// no clan
				//api.setClanInfo(0, false, "", 0, 0, 0);
				// has clan
				api.setClanInfo(1, true, "MY SUPER CLan!", 15, 666124, 1);
				
				api.addClanMemberInfo(100, "first", 1321, "234:23:23", "bandit", 342, 2323, 12, 14340, 0);
				for(var i=0; i<50; ++i)
					api.addClanMemberInfo(100+i+1, "blabla", 1321, "234:23:23", "bandit", 342, 2323, 12, 14340, 1);
				api.addClanMemberInfo(200, "last", 1321, "234:23:23", "bandit", 342, 2323, 12, 14340, 2);

				showClanList();
			}
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
			if (e.stageX > Clans.Mask.x && 
				e.stageX < Clans.Mask.x + Clans.Mask.width + 45 &&
				e.stageY > Clans.Mask.y && 
				e.stageY < Clans.Mask.y + Clans.Mask.height)
			{
				var dist = (Clans.Slots.height - Clans.Mask.height) / 48;
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


		public function showClanList()
		{
			if(api.myClanInfo.myClanID == 0) // no clan
			{
				Clans.Tab2.Text.Text.text = "$FR_CREATECLAN";
				Clans.Tab3.visible = false;
			}
			else
			{
				Clans.Tab2.Text.Text.text = "$FR_MYCLAN";
				Clans.Tab3.visible = false;
				if(api.myClanInfo.isAdmin)
				{
					Clans.Tab3.Text.Text.text = "$FR_MYCLANAPPLICATIONS";
					Clans.Tab3.visible = true;
				}
			}
			
			Clans.BtnJoin.visible = false;
			Clans.DescrBlock.visible = false;
			
			
			FrontEndEvents.eventRequestClanList(currentSort, 0);
			if(api.isDebug)
			{
				clearClanInfo();
				addClanInfo(0, "first clan", "First creator", 10, 1, "blald sdfldfk FIRST", 0);
				for(var i=0; i<50; ++i)
					addClanInfo(i+1, "just clan", "creator", 100, 10, "blald sdfldfk", 1);
				addClanInfo(100, "last clan", "Last creator", 1000, 100, "blald sdfldfk LAST", 2);
				
				populateClanList();
				
				//api.showClanInvite(10, "TEST INVITE CLAN", 666, "DUPER CLAN", 1);
			}
		}
		
		public function clearClanInfo()
		{
			clansInfo = new Array();
		}
		
		public function addClanInfo(clanID:uint, name:String, creator:String, xp:uint, numMembers:uint, description:String, iconID:uint)
		{
			clansInfo.push({clanID:clanID, name:name, creator:creator, xp:xp, numMembers:numMembers, description:description, iconID:iconID, movie:null});
		}
		
		public function populateClanList()
		{
			while (Clans.Slots.numChildren > 0)
			{
				Clans.Slots.removeChildAt(0);
			}			
			
			var y = 0;
			for(var i=0; i<clansInfo.length; ++i)
			{
				var slot = new warz.frontend.ClansSlot();
				clansInfo[i]["movie"] = slot;
				slot.clanInfo = clansInfo[i];
				slot.x = 0;
				slot.y = y;
				slot.Text.ClanName.text = clansInfo[i]["name"];
				slot.Text.Creator.text = clansInfo[i]["creator"];
				slot.Text.Members.text = clansInfo[i]["numMembers"];
				
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
												  for(var j=0; j<clansInfo.length; ++j)
												  {
													  if(clansInfo[j]["movie"].State == "active")
													  {
														  clansInfo[j]["movie"].State = "none";
														  clansInfo[j]["movie"].gotoAndPlay("out");
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
					selectedClanID = this.clanInfo["clanID"];
					Clans.DescrBlock.ClanName.text = this.clanInfo["name"];
					Clans.DescrBlock.Members.text = "$FR_CLANMEMBERS2 " + this.clanInfo["numMembers"];
					Clans.DescrBlock.Descr.text = this.clanInfo["description"];

					if (Clans.DescrBlock.Pic.numChildren > 0)
						Clans.DescrBlock.Pic.removeChildAt(0);
					loadSlotIcon (api.myClanIcons[this.clanInfo["iconID"]], Clans.DescrBlock.Pic, 0);
					
					Clans.BtnJoin.visible = true;
					Clans.DescrBlock.visible = true;
				}
				
				Clans.Slots.addChild(slot);
				
				y+=48;
			}
			
			if(clansInfo.length > 12)
			{
				Scroller.visible = true;
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = Clans.Mask.height - 4;
				Scroller.Field.height = (Clans.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				Clans.Slots.y = 318;
			}		
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = Clans.Slots.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = Clans.Mask.height;
			h -= mh;
			h /= h1;

			var	dest:Number = dist * h;
			var	step:Number = int (dest / 48);
			
			if (Clans.Slots.y != (318+48*step))
			{
				Tweener.addTween(Clans.Slots, {y:318+(48*step), time:api.tweenDelay, transition:"linear"});
			}

//			heroSlots.y = dist * h;
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