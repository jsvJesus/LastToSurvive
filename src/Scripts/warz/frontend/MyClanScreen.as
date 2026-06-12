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

	public class MyClanScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var MyClan:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;

		private var currentSort = "name";
		
		private var clanMembers:Array = new Array();
		private var selectedMember:int = 0;
		private var selectedBuySlotIdx:uint = 0;

		public	function MyClanScreen ()
		{
			visible = false;
			var me = this;
			
			Scroller = MyClan.Scroller;
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
			
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function(evt:MouseEvent) 
			{
				if(evt.buttonDown){
					var startY = me.Scroller.Field.y;
					var endY = me.Scroller.Field.height - me.Scroller.Trigger.height;
					me.ScrollerIsDragging = true;
					me.Scroller.Trigger.startDrag(false, new Rectangle (me.Scroller.Trigger.x, startY, 0, endY));
				}
			})
			
			MyClan.Tab1.Text.Text.text = "$FR_CLANS";
			MyClan.Tab2.Text.Text.text = "$FR_MYCLAN";
			MyClan.Tab3.Text.Text.text = "$FR_MYCLANAPPLICATIONS";
			
			MyClan.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyClan.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyClan.Tab1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyClan.Tab1.ActionFunction = function() {api.Main.showScreen("Clans");}

			MyClan.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyClan.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyClan.Tab2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyClan.Tab2.ActionFunction = function() 
			{
			}

			MyClan.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			MyClan.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			MyClan.Tab3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			MyClan.Tab3.ActionFunction = function() {api.Main.showScreen("MyClanAdmin");}

			MyClan.MenuBtnLeader.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.MenuBtnLeader.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.MenuBtnLeader.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.MenuBtnLeader.ActionFunction = function() {api.Main.showScreen("Leaderboard");}
			MyClan.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.MenuBtnFriends.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.MenuBtnFriends.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.MenuBtnFriends.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}
			MyClan.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.MenuBtnNews.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.MenuBtnNews.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.MenuBtnNews.ActionFunction = function() {api.showInfoMsg("$FR_ComingSoon", true);}
			
			MyClan.MenuBtnFriends.visible = false;
			MyClan.MenuBtnNews.visible = false;

			
			// leave clan
			MyClan.OptionsBlock1.Btn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.OptionsBlock1.Btn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.OptionsBlock1.Btn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.OptionsBlock1.Btn.ActionFunction = function() 
			{
				api.Main.MsgBox.showInfoOkCancelMsg("$FR_ClanLeaveClanAreYouSure", "", leaveClanCallback);
			}

			// buy slots
			MyClan.OptionsBlock2.Btn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.OptionsBlock2.Btn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.OptionsBlock2.Btn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.OptionsBlock2.Btn.ActionFunction = function() 
			{
				selectedBuySlotIdx = 0;
				MyClan.BuySlotsPopUp.Slot1.State = "active";
				MyClan.BuySlotsPopUp.Slot1.gotoAndPlay("pressed");
				MyClan.BuySlotsPopUp.Slot2.State = "none";
				MyClan.BuySlotsPopUp.Slot2.gotoAndStop(1);
				MyClan.BuySlotsPopUp.Slot3.State = "none";
				MyClan.BuySlotsPopUp.Slot3.gotoAndStop(1);
				MyClan.BuySlotsPopUp.Slot4.State = "none";
				MyClan.BuySlotsPopUp.Slot4.gotoAndStop(1);
				MyClan.BuySlotsPopUp.Slot5.State = "none";
				MyClan.BuySlotsPopUp.Slot5.gotoAndStop(1);
				MyClan.BuySlotsPopUp.Slot6.State = "none";
				MyClan.BuySlotsPopUp.Slot6.gotoAndStop(1);
				
				MyClan.BuySlotsPopUp.visible = true;
			}

			// donate gc
			/*MyClan.OptionsBlock3.Btn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.OptionsBlock3.Btn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.OptionsBlock3.Btn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.OptionsBlock3.Btn.ActionFunction = function() 
			{
				api.Main.MsgBox.showInfoInputMsg("$FR_ClanDonateGCHowMuch", "", gonateGCToClanCallback);
			}*/

			MyClan.ListTop.Btn2.Text.Text.text = "$FR_ClanPlName";
			MyClan.ListTop.Btn3.Text.Text.text = "$FR_ClanPlExp";
			MyClan.ListTop.Btn4.Text.Text.text = "$FR_ClanPlST";
			MyClan.ListTop.Btn5.Text.Text.text = "$FR_ClanPlRep";
			MyClan.ListTop.Btn6.Text.Text.text = "$FR_ClanPlKZ";
			MyClan.ListTop.Btn7.Text.Text.text = "$FR_ClanPlKS";
			MyClan.ListTop.Btn8.Text.Text.text = "$FR_ClanPlKB";
			//MyClan.ListTop.Btn9.Text.Text.text = "$FR_ClanPlDG";
			
			MyClan.AdminTools.BtnPromote.Text.Text.text = "$FR_ClanPromote";
			MyClan.AdminTools.BtnDemote.Text.Text.text = "$FR_ClanDemote";
			MyClan.AdminTools.BtnKick.Text.Text.text = "$FR_ClanKick";
			MyClan.AdminTools.BtnGiveGC.Text.Text.text = "$FR_ClanGiveGC";
			
			MyClan.AdminTools.BtnPromote.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.AdminTools.BtnPromote.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.AdminTools.BtnPromote.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.AdminTools.BtnPromote.ActionFunction = function() {
				api.showInfoOkCancelMsg("$FR_ClanPromoteAreYouSure" + " <"+clanMembers[selectedMember]["name"] + ">?","",function(isPromo: Boolean){
										if(isPromo)FrontEndEvents.eventClanAdminAction(clanMembers[selectedMember]["charID"], "promote");});}

			MyClan.AdminTools.BtnDemote.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.AdminTools.BtnDemote.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.AdminTools.BtnDemote.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.AdminTools.BtnDemote.ActionFunction = function() {
				api.showInfoOkCancelMsg("$FR_ClanDemoteAreYouSure" + " <"+clanMembers[selectedMember]["name"] + ">?","",function(isDemote: Boolean){
										if(isDemote)FrontEndEvents.eventClanAdminAction(clanMembers[selectedMember]["charID"], "demote");});}

			MyClan.AdminTools.BtnKick.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.AdminTools.BtnKick.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.AdminTools.BtnKick.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.AdminTools.BtnKick.ActionFunction = function() {
				api.showInfoOkCancelMsg("$FR_ClanKickAreYouSure" + " <"+clanMembers[selectedMember]["name"] + ">?","",function(isKick: Boolean){
										if(isKick)FrontEndEvents.eventClanAdminAction(clanMembers[selectedMember]["charID"], "kick");});}

			MyClan.AdminTools.BtnGiveGC.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.AdminTools.BtnGiveGC.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.AdminTools.BtnGiveGC.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.AdminTools.BtnGiveGC.ActionFunction = function() 
			{
				api.Main.MsgBox.showInfoInputMsg("$FR_ClanGiveGCHowMuch", "", giveGCToMemberCallback);
			}
			
			MyClan.BuySlotsPopUp.CancelBtn.Text.Text.text = "$FR_Cancel";
			MyClan.BuySlotsPopUp.CancelBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.BuySlotsPopUp.CancelBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.BuySlotsPopUp.CancelBtn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.BuySlotsPopUp.CancelBtn.ActionFunction = function() 
			{
				MyClan.BuySlotsPopUp.visible = false;
			}

			MyClan.BuySlotsPopUp.ApplyBtn.Text.Text.text = "$FR_Buy";
			MyClan.BuySlotsPopUp.ApplyBtn.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			MyClan.BuySlotsPopUp.ApplyBtn.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			MyClan.BuySlotsPopUp.ApplyBtn.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn);
			MyClan.BuySlotsPopUp.ApplyBtn.ActionFunction = function() 
			{
				MyClan.BuySlotsPopUp.visible = false;
				FrontEndEvents.eventClanBuySlots(selectedBuySlotIdx);
			}
			
			MyClan.BuySlotsPopUp.Slot1.buyIdx = 0;
			MyClan.BuySlotsPopUp.Slot1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			MyClan.BuySlotsPopUp.Slot1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			MyClan.BuySlotsPopUp.Slot1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			MyClan.BuySlotsPopUp.Slot2.buyIdx = 1;
			MyClan.BuySlotsPopUp.Slot2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			MyClan.BuySlotsPopUp.Slot2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			MyClan.BuySlotsPopUp.Slot2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			MyClan.BuySlotsPopUp.Slot3.buyIdx = 2;
			MyClan.BuySlotsPopUp.Slot3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			MyClan.BuySlotsPopUp.Slot3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			MyClan.BuySlotsPopUp.Slot3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			MyClan.BuySlotsPopUp.Slot4.buyIdx = 3;
			MyClan.BuySlotsPopUp.Slot4.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			MyClan.BuySlotsPopUp.Slot4.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			MyClan.BuySlotsPopUp.Slot4.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			MyClan.BuySlotsPopUp.Slot5.buyIdx = 4;
			MyClan.BuySlotsPopUp.Slot5.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			MyClan.BuySlotsPopUp.Slot5.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			MyClan.BuySlotsPopUp.Slot5.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			MyClan.BuySlotsPopUp.Slot6.buyIdx = 5;
			MyClan.BuySlotsPopUp.Slot6.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			MyClan.BuySlotsPopUp.Slot6.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			MyClan.BuySlotsPopUp.Slot6.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn3);
			
			MyClan.BuySlotsPopUp.Slot1.ActionFunction = MyClan.BuySlotsPopUp.Slot2.ActionFunction = 
			MyClan.BuySlotsPopUp.Slot3.ActionFunction = MyClan.BuySlotsPopUp.Slot4.ActionFunction = 
			MyClan.BuySlotsPopUp.Slot5.ActionFunction = MyClan.BuySlotsPopUp.Slot6.ActionFunction = 
			function() 
			{
				selectedBuySlotIdx = this.buyIdx;
			}
		}
		
		public	function leaveClanCallback (state:Boolean):void
		{
			if (state)
			{
				FrontEndEvents.eventClanLeaveClan();
			}
		}
		public	function gonateGCToClanCallback (state:Boolean, text:String=""):void
		{
			if (state)
			{
				FrontEndEvents.eventClanDonateGCToClan(uint(text));
			}
		}
		public	function giveGCToMemberCallback (state:Boolean, text:String=""):void
		{
			if (state)
			{
				FrontEndEvents.eventClanAdminDonateGC(clanMembers[selectedMember]["charID"], uint(text));
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
				for(var i =1; i<5; ++i)
				{
					var n = "Slot"+i;
					if(MyClan.BuySlotsPopUp[n].State == "active")
					{
						MyClan.BuySlotsPopUp[n].State = "none";
						MyClan.BuySlotsPopUp[n].gotoAndPlay("out");
					}
				}
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction();
			}
		}

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			var	survivor:Survivor = api.Survivors[api.SelectedChar];
			
			MyClan.BuySlotsPopUp.visible = false;
			MyClan.BuySlotsPopUp.Slot1.Text.Slots.text = api.myClanSlotsBuyInfo[0]["numSlots"]+" $FR_Slots";
			MyClan.BuySlotsPopUp.Slot1.Text.Price.text = api.myClanSlotsBuyInfo[0]["price"]+" $FR_GC";
			MyClan.BuySlotsPopUp.Slot2.Text.Slots.text = api.myClanSlotsBuyInfo[1]["numSlots"]+" $FR_Slots";
			MyClan.BuySlotsPopUp.Slot2.Text.Price.text = api.myClanSlotsBuyInfo[1]["price"]+" $FR_GC";
			MyClan.BuySlotsPopUp.Slot3.Text.Slots.text = api.myClanSlotsBuyInfo[2]["numSlots"]+" $FR_Slots";
			MyClan.BuySlotsPopUp.Slot3.Text.Price.text = api.myClanSlotsBuyInfo[2]["price"]+" $FR_GC";
			MyClan.BuySlotsPopUp.Slot4.Text.Slots.text = api.myClanSlotsBuyInfo[3]["numSlots"]+" $FR_Slots";
			MyClan.BuySlotsPopUp.Slot4.Text.Price.text = api.myClanSlotsBuyInfo[3]["price"]+" $FR_GC";
			MyClan.BuySlotsPopUp.Slot5.Text.Slots.text = api.myClanSlotsBuyInfo[4]["numSlots"]+" $FR_Slots";
			MyClan.BuySlotsPopUp.Slot5.Text.Price.text = api.myClanSlotsBuyInfo[4]["price"]+" $FR_GC";
			MyClan.BuySlotsPopUp.Slot6.Text.Slots.text = api.myClanSlotsBuyInfo[5]["numSlots"]+" $FR_Slots";
			MyClan.BuySlotsPopUp.Slot6.Text.Price.text = api.myClanSlotsBuyInfo[5]["price"]+" $FR_GC";
			
			currentSort = "name";

			MyClan.Tab2.gotoAndPlay("pressed");
			MyClan.Tab2.State = "active";
			MyClan.MenuBtnClans.gotoAndPlay("pressed");
			
			MyClan.Tab3.visible = false;
			if(api.myClanInfo.isAdmin)
			{
				MyClan.Tab3.Text.Text.text = "$FR_MYCLANAPPLICATIONS";
				MyClan.Tab3.visible = true;
			}
			
			MyClan.ClanName.text = api.myClanInfo.myName;
			MyClan.OptionsBlock2.Slots.text = api.myClanInfo.availableSlots;
			//MyClan.OptionsBlock3.GC.text = api.myClanInfo.clanReserve + " $FR_GC";

			if (MyClan.OptionsBlock1.Pic.numChildren > 0)
				MyClan.OptionsBlock1.Pic.removeChildAt(0);
			loadSlotIcon(api.myClanIcons[api.myClanInfo.logoID], MyClan.OptionsBlock1.Pic, 0);

			showClanMembers();

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
			if (e.stageX > MyClan.Mask.x && 
				e.stageX < MyClan.Mask.x + MyClan.Mask.width + 45 &&
				e.stageY > MyClan.Mask.y && 
				e.stageY < MyClan.Mask.y + MyClan.Mask.height)
			{
				var dist = (MyClan.Slots.height - MyClan.Mask.height) / 48;
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

		public function showClanMembers()
		{
			MyClan.AdminTools.visible = false;
			
			while (MyClan.Slots.numChildren > 0)
			{
				MyClan.Slots.removeChildAt(0);
			}			
			
			clanMembers = api.myClanInfo.clanMembers.slice();

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
			for(var i=0; i<clanMembers.length; ++i)
			{
				var slot = new warz.frontend.ClansMyClanSlot();
				clanMembers[i]["movie"] = slot;
				slot.memberInfo = clanMembers[i];
				slot.memberID = i;
				slot.x = 0;
				slot.y = y;
				slot.MemberIcons.gotoAndStop(clanMembers[i]["rank"]+1);
				slot.Text.P.text = i+1;
				slot.Text.Name.text = clanMembers[i]["name"];
				slot.Text.Exp.text = clanMembers[i]["exp"];
				slot.Text.Time.text = clanMembers[i]["time"];
				slot.Text.Reputation.text = clanMembers[i]["rep"];
				slot.Text.KilledZombies.text = clanMembers[i]["kzombie"];
				slot.Text.KilledSurvivors.text = clanMembers[i]["ksurvivor"];
				slot.Text.KilledBandits.text = clanMembers[i]["kbandits"];
				//slot.Text.DonatedGC.text = clanMembers[i]["donatedgc"];
				
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
												  for(var j=0; j<clanMembers.length; ++j)
												  {
													  if(clanMembers[j]["movie"].State == "active")
													  {
														  clanMembers[j]["movie"].State = "none";
														  clanMembers[j]["movie"].gotoAndPlay("out");
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
					selectedMember = this.memberID;
					if(api.myClanInfo.isAdmin)
					{
						MyClan.AdminTools.visible = true;
					}
				}
				
				MyClan.Slots.addChild(slot);
				
				y+=48;
			}
			
			if(clanMembers.length > 8)
			{
				Scroller.visible = true;
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = MyClan.Mask.height - 4;
				Scroller.Field.height = (MyClan.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				MyClan.Slots.y = 601;
			}				
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = MyClan.Slots.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = MyClan.Mask.height;
			h -= mh;
			h /= h1;

			var	dest:Number = dist * h;
			var	step:Number = int (dest / 48);
			
			if (MyClan.Slots.y != (601+48*step))
			{
				Tweener.addTween(MyClan.Slots, {y:601+(48*step), time:api.tweenDelay, transition:"linear"});
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
			
			slotWidth = 256;
			slotHeight = 256;
			slotX = 0;
			slotY = 0;
			
			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = slotX; 
			bitmap.y = slotY; 
		
			dat.imageHolder.addChild (bitmap);
		}
	}
}