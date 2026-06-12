package warz.frontend {
	import flash.display.MovieClip;
	import flash.utils.*;
	import warz.dataObjects.*;
	import flash.events.*;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import caurina.transitions.Tweener;

	public class BrowseGamesScreen extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public var api:warz.frontend.Frontend=null;
		
		public var ServBrowse:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;
		
		private var TabArray:Array = new Array();
		private var GameList:Array = null;
		private var SelectedGameID:int = 0;
		private var currentTabName:String = "browse";
		
		private var currentSort = "name";
		private var currentSortOrder = 1;
		
		public	function BrowseGamesScreen ()
		{
			visible = false;
			var me = this;
			
			Scroller = ServBrowse.Scroller;
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
			
			ServBrowse.BtnRentServer.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ServBrowse.BtnRentServer.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ServBrowse.BtnRentServer.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			ServBrowse.BtnRentServer.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			ServBrowse.BtnResetFilters.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ServBrowse.BtnResetFilters.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ServBrowse.BtnResetFilters.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			ServBrowse.BtnResetFilters.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			ServBrowse.BtnJoinServer.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ServBrowse.BtnJoinServer.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ServBrowse.BtnJoinServer.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			ServBrowse.BtnJoinServer.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			ServBrowse.BtnDonateGC.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ServBrowse.BtnDonateGC.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ServBrowse.BtnDonateGC.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			ServBrowse.BtnDonateGC.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			
			ServBrowse.BtnDonateGC.Text.Text.text = "$FR_ClanDonateGC";

			ServBrowse.BtnRentServer.visible = false;
			ServBrowse.BtnRentServer.ActionFunction = function()
			{
				api.Main.showScreen("MyServers");
			}
			ServBrowse.BtnResetFilters.ActionFunction = function()
			{
				FrontEndEvents.eventBrowseGamesSetFilter(api.BrowseGames_Region_US, api.BrowseGames_Region_EU, api.BrowseGames_Region_RU, api.BrowseGames_Region_SA,
														 api.BrowseGames_Filter_Gameworld, api.BrowseGames_Filter_Stronghold, api.BrowseGames_Filter_HideEmpty, api.BrowseGames_Filter_HideFull, api.BrowseGames_Filter_PrivateServers,
														 api.BrowseGames_Options_Tracers, api.BrowseGames_Options_Nameplates, api.BrowseGames_Options_Crosshair,
														 ServBrowse.Filters.NameFilterInput.text, api.BrowseGames_Options_Enable,
														 api.BrowseGames_Options_Password,
														 uint(ServBrowse.Filters.InputTimeLimit.text));
				showGames(currentTabName, 0);
			}
			ServBrowse.BtnJoinServer.ActionFunction = function()
			{
				FrontEndEvents.eventBrowseGamesJoin(SelectedGameID);
			}
			ServBrowse.BtnDonateGC.ActionFunction = function()
			{
				FrontEndEvents.eventRequestShowDonateGCtoServer(SelectedGameID);
				//api.Main.DonateGCSrvPopUp.showPopUp(SelectedGameID);
			}

			ServBrowse.BtnNavLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ServBrowse.BtnNavLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ServBrowse.BtnNavLeft.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			ServBrowse.BtnNavLeft.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			ServBrowse.BtnNavTop.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ServBrowse.BtnNavTop.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ServBrowse.BtnNavTop.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			ServBrowse.BtnNavTop.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			ServBrowse.BtnNavRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			ServBrowse.BtnNavRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			ServBrowse.BtnNavRight.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			ServBrowse.BtnNavRight.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			ServBrowse.BtnNavLeft.ActionFunction = function()
			{
				showGames(currentTabName, -1);
			}
			ServBrowse.BtnNavTop.ActionFunction = function()
			{
				showGames(currentTabName, 0);
			}
			ServBrowse.BtnNavRight.ActionFunction = function()
			{
				showGames(currentTabName, 1);
			}

			TabArray.push(ServBrowse.Tab1);
			TabArray.push(ServBrowse.Tab2);
			TabArray.push(ServBrowse.Tab3);
			
			ServBrowse.Tab1.Text.Text.text = "$FR_BROWSE_BROWSE";
			ServBrowse.Tab2.Text.Text.text = "$FR_BROWSE_RECENT";
			ServBrowse.Tab3.Text.Text.text = "$FR_BROWSE_FAVORITES";
			ServBrowse.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OVER, TabRollOverFn);
			ServBrowse.Tab1.Btn.addEventListener(MouseEvent.MOUSE_OUT, TabRollOutFn);
			ServBrowse.Tab1.Btn.addEventListener(MouseEvent.MOUSE_DOWN, TabPressFn);
			ServBrowse.Tab1.Btn.addEventListener(MouseEvent.MOUSE_UP, TabPressUpFn);
			ServBrowse.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OVER, TabRollOverFn);
			ServBrowse.Tab2.Btn.addEventListener(MouseEvent.MOUSE_OUT, TabRollOutFn);
			ServBrowse.Tab2.Btn.addEventListener(MouseEvent.MOUSE_DOWN, TabPressFn);
			ServBrowse.Tab2.Btn.addEventListener(MouseEvent.MOUSE_UP, TabPressUpFn);
			ServBrowse.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OVER, TabRollOverFn);
			ServBrowse.Tab3.Btn.addEventListener(MouseEvent.MOUSE_OUT, TabRollOutFn);
			ServBrowse.Tab3.Btn.addEventListener(MouseEvent.MOUSE_DOWN, TabPressFn);
			ServBrowse.Tab3.Btn.addEventListener(MouseEvent.MOUSE_UP, TabPressUpFn);
			
			ServBrowse.Tab1.ActionFunction = function()
			{
				showGames("browse", 0);
			}
			ServBrowse.Tab2.ActionFunction = function()
			{
				showGames("recent", 0);
			}
			ServBrowse.Tab3.ActionFunction = function()
			{
				showGames("favorites", 0);
			}
			
			ServBrowse.Filters.RadiobtnRegion1.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.RadiobtnRegion1.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.RadiobtnRegion2.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.RadiobtnRegion2.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.RadiobtnRegion3.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.RadiobtnRegion3.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.RadiobtnRegion4.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.RadiobtnRegion4.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);

			//ServBrowse.Filters.CheckboxFilter1.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			//ServBrowse.Filters.CheckboxFilter1.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			//ServBrowse.Filters.CheckboxFilter2.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			//ServBrowse.Filters.CheckboxFilter2.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.CheckboxFilter3.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.CheckboxFilter3.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.CheckboxFilter4.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.CheckboxFilter4.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			//ServBrowse.Filters.CheckboxFilter5.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			//ServBrowse.Filters.CheckboxFilter5.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);

			ServBrowse.Filters.CheckboxServerOption1.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.CheckboxServerOption1.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.CheckboxServerOption2.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.CheckboxServerOption2.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.CheckboxServerOption3.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.CheckboxServerOption3.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.CheckboxServerOption4.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.CheckboxServerOption4.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);
			ServBrowse.Filters.CheckboxServerOptions.Btn.addEventListener(MouseEvent.MOUSE_OVER, CheckboxRollOverFn);
			ServBrowse.Filters.CheckboxServerOptions.Btn.addEventListener(MouseEvent.MOUSE_OUT, CheckboxRollOutFn);

			//ServBrowse.Filters.CheckboxFilter1.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			//ServBrowse.Filters.CheckboxFilter1.ActionFunction = function(flag:Boolean) { api.BrowseGames_Filter_Gameworld=flag; }
			//ServBrowse.Filters.CheckboxFilter2.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			//ServBrowse.Filters.CheckboxFilter2.ActionFunction = function(flag:Boolean) { api.BrowseGames_Filter_Stronghold=flag; }
			ServBrowse.Filters.CheckboxFilter3.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			ServBrowse.Filters.CheckboxFilter3.ActionFunction = function(flag:Boolean) { api.BrowseGames_Filter_HideEmpty=flag; }
			ServBrowse.Filters.CheckboxFilter4.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			ServBrowse.Filters.CheckboxFilter4.ActionFunction = function(flag:Boolean) { api.BrowseGames_Filter_HideFull=flag; }
			//ServBrowse.Filters.CheckboxFilter5.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			//ServBrowse.Filters.CheckboxFilter5.ActionFunction = function(flag:Boolean) { api.BrowseGames_Filter_PrivateServers=flag; }
		
			ServBrowse.Filters.CheckboxServerOption1.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			ServBrowse.Filters.CheckboxServerOption1.ActionFunction = function(flag:Boolean) { api.BrowseGames_Options_Tracers=flag; }
			ServBrowse.Filters.CheckboxServerOption2.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			ServBrowse.Filters.CheckboxServerOption2.ActionFunction = function(flag:Boolean) { api.BrowseGames_Options_Nameplates=flag; }
			ServBrowse.Filters.CheckboxServerOption3.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			ServBrowse.Filters.CheckboxServerOption3.ActionFunction = function(flag:Boolean) { api.BrowseGames_Options_Crosshair=flag; }
			ServBrowse.Filters.CheckboxServerOption4.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			ServBrowse.Filters.CheckboxServerOption4.ActionFunction = function(flag:Boolean) { api.BrowseGames_Options_Password=flag; }
			ServBrowse.Filters.CheckboxServerOptions.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn);
			ServBrowse.Filters.CheckboxServerOptions.ActionFunction = function(flag:Boolean) { 
				api.BrowseGames_Options_Enable=flag; 
				ServBrowse.Filters.gotoAndStop(api.BrowseGames_Options_Enable?"so_on":"so_off");
				updateFilterTextHack();
			}

			ServBrowse.Filters.RadiobtnRegion1.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn2);
			ServBrowse.Filters.RadiobtnRegion1.ActionFunction = function(flag:Boolean) { api.BrowseGames_Region_US=flag; }
			ServBrowse.Filters.RadiobtnRegion2.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn2);
			ServBrowse.Filters.RadiobtnRegion2.ActionFunction = function(flag:Boolean) { api.BrowseGames_Region_EU=flag; }
			ServBrowse.Filters.RadiobtnRegion3.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn2);
			ServBrowse.Filters.RadiobtnRegion3.ActionFunction = function(flag:Boolean) { api.BrowseGames_Region_RU=flag; }
			ServBrowse.Filters.RadiobtnRegion4.Btn.addEventListener(MouseEvent.CLICK, CheckboxClickFn2);
			ServBrowse.Filters.RadiobtnRegion4.ActionFunction = function(flag:Boolean) { api.BrowseGames_Region_SA=flag; }
			
			ServBrowse.SlotlistTop.BtnSort1.Btn.addEventListener(MouseEvent.MOUSE_OVER, SortBtnRollOverFn);
			ServBrowse.SlotlistTop.BtnSort1.Btn.addEventListener(MouseEvent.MOUSE_OUT, SortBtnRollOutFn);
			ServBrowse.SlotlistTop.BtnSort2.Btn.addEventListener(MouseEvent.MOUSE_OVER, SortBtnRollOverFn);
			ServBrowse.SlotlistTop.BtnSort2.Btn.addEventListener(MouseEvent.MOUSE_OUT, SortBtnRollOutFn);
			ServBrowse.SlotlistTop.BtnSort3.Btn.addEventListener(MouseEvent.MOUSE_OVER, SortBtnRollOverFn);
			ServBrowse.SlotlistTop.BtnSort3.Btn.addEventListener(MouseEvent.MOUSE_OUT, SortBtnRollOutFn);
			ServBrowse.SlotlistTop.BtnSort4.Btn.addEventListener(MouseEvent.MOUSE_OVER, SortBtnRollOverFn);
			ServBrowse.SlotlistTop.BtnSort4.Btn.addEventListener(MouseEvent.MOUSE_OUT, SortBtnRollOutFn);
			ServBrowse.SlotlistTop.BtnSort1.Btn.addEventListener(MouseEvent.CLICK, SortBtnClickFn);
			ServBrowse.SlotlistTop.BtnSort2.Btn.addEventListener(MouseEvent.CLICK, SortBtnClickFn);
			ServBrowse.SlotlistTop.BtnSort3.Btn.addEventListener(MouseEvent.CLICK, SortBtnClickFn);
			ServBrowse.SlotlistTop.BtnSort4.Btn.addEventListener(MouseEvent.CLICK, SortBtnClickFn);
			ServBrowse.SlotlistTop.BtnSort1.sortName ="name";
			ServBrowse.SlotlistTop.BtnSort2.sortName ="mode";
			ServBrowse.SlotlistTop.BtnSort3.sortName ="map";
			ServBrowse.SlotlistTop.BtnSort4.sortName ="ping";
		}
		public function updateFilterTextHack()
		{
			ServBrowse.Filters.Region1.text = "$FR_REGION_NA";
			ServBrowse.Filters.Region2.text = "$FR_REGION_EU";
			ServBrowse.Filters.Region3.text = "$FR_REGION_RU";
			ServBrowse.Filters.Region4.text = "$FR_REGION_SA";
			
			//ServBrowse.Filters.Filter1.text = "$FR_GAMEWORLDS";
			//ServBrowse.Filters.Filter2.text = "$FR_STRONGHOLDS";
			ServBrowse.Filters.Filter3.text = "$FR_HIDE_EMPTY";
			ServBrowse.Filters.Filter4.text = "$FR_HIDE_FULL";
			//ServBrowse.Filters.Filter5.text = "$FR_SHOW_PRIVATE_SERVERS";

			ServBrowse.Filters.ServerOption1.text = "$FR_TRACERS";
			ServBrowse.Filters.ServerOption2.text = "$FR_NAMEPLATES";
			ServBrowse.Filters.ServerOption3.text = "$FR_CROSSHAIR";
			ServBrowse.Filters.ServerOption4.text = "$FR_PASSWORD_PROTECTED";
			ServBrowse.Filters.ServerOption5.text = "$FR_GametimeLimit";
		}
		private function SortBtnRollOverFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("over");
		}
		private function SortBtnRollOutFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("out");
		}
		private function SortBtnClickFn(evt:Event)
		{
			evt.currentTarget.parent.gotoAndPlay("pressed");
			SoundEvents.eventSoundPlay("menu_click");
			
			if(currentSort == evt.currentTarget.parent.sortName)
			{
				if(currentSortOrder == 1)
					currentSortOrder = -1;
				else
					currentSortOrder = 1;
			}
			else
			{
				currentSort = evt.currentTarget.parent.sortName;
				currentSortOrder = 1;
			}
			showGames(currentTabName, 0);
		}
		
		private function CheckboxRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel!="pressed")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		private function CheckboxRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel!="pressed")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		private function CheckboxClickFn(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
			if(evt.currentTarget.parent.currentLabel!="pressed")
			{
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction(true);
			}
			else
			{
				evt.currentTarget.parent.gotoAndStop(6);
				evt.currentTarget.parent.ActionFunction(false);
			}
		}
		private function CheckboxClickFn2(evt:Event)
		{
			SoundEvents.eventSoundPlay("menu_click");
			if(evt.currentTarget.parent.currentLabel!="pressed")
			{
				evt.currentTarget.parent.gotoAndPlay("pressed");
				evt.currentTarget.parent.ActionFunction(true);
			}
			else
			{
				evt.currentTarget.parent.gotoAndStop();
				evt.currentTarget.parent.ActionFunction(false);
			}
			if(evt.currentTarget.parent == ServBrowse.Filters.RadiobtnRegion1)
			{
				if(ServBrowse.Filters.RadiobtnRegion2.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion2.gotoAndPlay("out");
				if(ServBrowse.Filters.RadiobtnRegion3.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion3.gotoAndPlay("out");
				if(ServBrowse.Filters.RadiobtnRegion4.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion4.gotoAndPlay("out");
				api.BrowseGames_Region_EU = false;
				api.BrowseGames_Region_RU = false;
				api.BrowseGames_Region_SA = false;
			}
			if(evt.currentTarget.parent == ServBrowse.Filters.RadiobtnRegion2)
			{
				if(ServBrowse.Filters.RadiobtnRegion1.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion1.gotoAndPlay("out");
				if(ServBrowse.Filters.RadiobtnRegion3.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion3.gotoAndPlay("out");
				if(ServBrowse.Filters.RadiobtnRegion4.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion4.gotoAndPlay("out");
				api.BrowseGames_Region_US = false;
				api.BrowseGames_Region_RU = false;
				api.BrowseGames_Region_SA = false;
			}
			if(evt.currentTarget.parent == ServBrowse.Filters.RadiobtnRegion3)
			{
				if(ServBrowse.Filters.RadiobtnRegion1.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion1.gotoAndPlay("out");
				if(ServBrowse.Filters.RadiobtnRegion2.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion2.gotoAndPlay("out");
				if(ServBrowse.Filters.RadiobtnRegion4.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion4.gotoAndPlay("out");
				api.BrowseGames_Region_US = false;
				api.BrowseGames_Region_EU = false;
				api.BrowseGames_Region_SA = false;
			}
			if(evt.currentTarget.parent == ServBrowse.Filters.RadiobtnRegion4)
			{
				if(ServBrowse.Filters.RadiobtnRegion1.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion1.gotoAndPlay("out");
				if(ServBrowse.Filters.RadiobtnRegion2.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion2.gotoAndPlay("out");
				if(ServBrowse.Filters.RadiobtnRegion3.currentLabel == "pressed")
					ServBrowse.Filters.RadiobtnRegion3.gotoAndPlay("out");
				api.BrowseGames_Region_US = false;
				api.BrowseGames_Region_EU = false;
				api.BrowseGames_Region_RU = false;
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

		private function TabRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		private function TabRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		private function TabPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				for(var i=0; i<TabArray.length; ++i)
				{
					if(TabArray[i].State == "active")
					{
						TabArray[i].State = "none";
						TabArray[i].gotoAndPlay("out");
					}
				}
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
			}
		}
		private function TabPressUpFn(evt:Event)
		{
			if(evt.currentTarget.parent.State == "active")
				evt.currentTarget.parent.ActionFunction();
		}

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			if(api.isDebug==false)
			{
				while(ServBrowse.List.numChildren > 0)
					ServBrowse.List.removeChildAt(0);
			}
			FrontEndEvents.eventBrowseGamesRequestFilterStatus();
			
			ServBrowse.Filters.InputTimeLimit.text = "0";
			
			//if(api.BrowseGames_Filter_Gameworld)
			//	ServBrowse.Filters.CheckboxFilter1.gotoAndPlay("pressed");
			//if(api.BrowseGames_Filter_Stronghold)
			//	ServBrowse.Filters.CheckboxFilter2.gotoAndPlay("pressed");
			if(api.BrowseGames_Filter_HideEmpty)
				ServBrowse.Filters.CheckboxFilter3.gotoAndPlay("pressed");
			if(api.BrowseGames_Filter_HideFull)
				ServBrowse.Filters.CheckboxFilter4.gotoAndPlay("pressed");
			//if(api.BrowseGames_Filter_PrivateServers)
			//	ServBrowse.Filters.CheckboxFilter5.gotoAndPlay("pressed");

			if(api.BrowseGames_Options_Tracers)
				ServBrowse.Filters.CheckboxServerOption1.gotoAndPlay("pressed");
			if(api.BrowseGames_Options_Nameplates)
				ServBrowse.Filters.CheckboxServerOption2.gotoAndPlay("pressed");
			if(api.BrowseGames_Options_Crosshair)
				ServBrowse.Filters.CheckboxServerOption3.gotoAndPlay("pressed");
			if(api.BrowseGames_Options_Password)
				ServBrowse.Filters.CheckboxServerOption4.gotoAndPlay("pressed");
			if(api.BrowseGames_Options_Enable)
				ServBrowse.Filters.CheckboxServerOptions.gotoAndPlay("pressed");
			ServBrowse.Filters.gotoAndStop(api.BrowseGames_Options_Enable?"so_on":"so_off");
			updateFilterTextHack();
			
			if(api.BrowseGames_Region_US)
				ServBrowse.Filters.RadiobtnRegion1.gotoAndPlay("pressed");
			if(api.BrowseGames_Region_EU)
				ServBrowse.Filters.RadiobtnRegion2.gotoAndPlay("pressed");
			if(api.BrowseGames_Region_RU)
				ServBrowse.Filters.RadiobtnRegion3.gotoAndPlay("pressed");
			if(api.BrowseGames_Region_SA)
				ServBrowse.Filters.RadiobtnRegion4.gotoAndPlay("pressed");
			
			ServBrowse.SlotlistTop.Arrow1.visible = false;
			ServBrowse.SlotlistTop.Arrow2.visible = false;
			ServBrowse.SlotlistTop.Arrow3.visible = false;
			ServBrowse.SlotlistTop.Arrow4.visible = false;

			currentSort = "name";
			currentSortOrder = 1;

			ServBrowse.Tab1.State = "active";
			ServBrowse.Tab1.gotoAndPlay("pressed");
			ServBrowse.Tab2.State = "";
			ServBrowse.Tab3.State = "";
			ServBrowse.Tab2.gotoAndStop(1);
			ServBrowse.Tab3.gotoAndStop(1);
			showGames("browse", 0);

			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
			this.visible = true;
		}
		
		public function Deactivate()
		{
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);
			this.visible = false;
		}
		
		public function showGames(type:String, op:int)
		{
			GameList = new Array();
			currentTabName = type;
			ServBrowse.BtnJoinServer.visible = false;
			ServBrowse.BtnDonateGC.visible = false;
			SelectedGameID = 0;
			FrontEndEvents.eventBrowseGamesRequestList(type, currentSort, currentSortOrder, op);
			if(api.isDebug)
			{
				addGameToList(0, "name1", "mode1", "map1", true, false, true, "13/54", 1, true, false, false, true, true, true);
				for(var i=1; i<50; ++i)
					addGameToList(i, "nameX", "modeX", "mapX", false, true, false, "XX/YY", 50, false, true, true, false, false, false);
				addGameToList(60, "nameLast", "modeLast", "mapLast", false, false, false, "100/104", 100, true, false, false, true, true, true);
				
				showGameList();
			}
		}
		
		public function addGameToList(id:Number, name:String, mode:String, map:String, tracers:Boolean, nametags:Boolean, crosshair:Boolean, players:String, ping:int, favorite:Boolean, isPassword:Boolean, isTimeLimit:Boolean, trialsAllowed:Boolean, donate:Boolean, disableWeapon:Boolean)
		{
			GameList.push({id:id, name:name, mode:mode, map:map, tracers:tracers, nametags:nametags, crosshair:crosshair, players:players, ping:ping, movie:null, favorite:favorite, isPassword:isPassword, isTimeLimit:isTimeLimit, trialsAllowed:trialsAllowed, donate:donate, disableWeapon:disableWeapon});
		}
		
		/*private function sortByName(a, b) :Number
		{
			var n1 = a["name"];
			var n2 = b["name"];
			
			if(n1 > n2) return currentSortOrder;
			else if(n1 < n2) return -currentSortOrder;
			else return 0;			
		}
		private function sortByMode(a, b) :Number
		{
			var n1 = a["mode"];
			var n2 = b["mode"];
			
			if(n1 > n2) return currentSortOrder;
			else if(n1 < n2) return -currentSortOrder;
			else return 0;			
		}
		private function sortByMap(a, b) :Number
		{
			var n1 = a["map"];
			var n2 = b["map"];
			
			if(n1 > n2) return currentSortOrder;
			else if(n1 < n2) return -currentSortOrder;
			else return 0;			
		}
		private function sortByPing(a, b) :Number
		{
			var n1 = a["ping"];
			var n2 = b["ping"];
			
			if(n1 > n2) return currentSortOrder;
			else if(n1 < n2) return -currentSortOrder;
			else return 0;			
		}*/
		
		public function showGameList()
		{
			var me = this;
			
			while(ServBrowse.List.numChildren > 0)
				ServBrowse.List.removeChildAt(0);
			
			ServBrowse.SlotlistTop.Arrow1.visible = false;
			ServBrowse.SlotlistTop.Arrow2.visible = false;
			ServBrowse.SlotlistTop.Arrow3.visible = false;
			ServBrowse.SlotlistTop.Arrow4.visible = false;

			if(currentSort == "name")
			{
				//GameList.sort(sortByName);
				ServBrowse.SlotlistTop.Arrow1.visible = true;
				ServBrowse.SlotlistTop.Arrow1.rotation = -90*currentSortOrder;
			}
			else if(currentSort == "mode")
			{
				//GameList.sort(sortByMode);
				ServBrowse.SlotlistTop.Arrow2.visible = true;
				ServBrowse.SlotlistTop.Arrow2.rotation = -90*currentSortOrder;
			}
			else if(currentSort == "map")
			{
				//GameList.sort(sortByMap);
				ServBrowse.SlotlistTop.Arrow3.visible = true;
				ServBrowse.SlotlistTop.Arrow3.rotation = -90*currentSortOrder;
			}
			else if(currentSort == "ping")
			{
				//GameList.sort(sortByPing);
				ServBrowse.SlotlistTop.Arrow4.visible = true;
				ServBrowse.SlotlistTop.Arrow4.rotation = -90*currentSortOrder;
			}
			
			var slotY = 0;
			for(var i=0; i<GameList.length; ++i)
			{
				var slot = new warz.frontend.ServBrowseSlot();
				GameList[i]["movie"] = slot;

				slot.x = 0;
				slot.y = slotY; slotY += 47;
				
				slot.gameID = GameList[i]["id"];	
				slot.DonateAllowed = GameList[i]["donate"];
				slot.Text.Name.text = GameList[i]["name"];
				slot.Text.Mode.text = GameList[i]["mode"];
				slot.Text.Map.text = GameList[i]["map"];
				slot.Text.Players.text = GameList[i]["players"];
				slot.Ping.gotoAndStop(GameList[i]["ping"]);
				slot.IconTracer.gotoAndStop(GameList[i]["tracers"]?"on":"off");
				slot.IconNameplate.gotoAndStop(GameList[i]["nametags"]?"on":"off");
				slot.IconCrosshair.gotoAndStop(GameList[i]["crosshair"]?"on":"off");
				slot.IconLock.visible = GameList[i]["isPassword"];
				slot.IconTime.visible = GameList[i]["isTimeLimit"];
				slot.IconTrial.visible = !GameList[i]["trialsAllowed"];
				slot.IconNoWeapons.visible = GameList[i]["disableWeapon"];
				slot.BtnFav.visible = !GameList[i]["favorite"];
				slot.BtnRem.visible = GameList[i]["favorite"];
				slot.BtnFav.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event)
												 {
													 evt.currentTarget.parent.gotoAndPlay("over");
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.MOUSE_OVER));
												 });
				slot.BtnFav.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event)
												 {
													 evt.currentTarget.parent.gotoAndPlay("out");
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.MOUSE_OUT));
												 });
				slot.BtnFav.Btn.addEventListener(MouseEvent.MOUSE_DOWN, function(evt:Event)
												 {
													 SoundEvents.eventSoundPlay("menu_click");
													 evt.currentTarget.parent.gotoAndPlay("pressed");
												 });
				slot.BtnFav.Btn.addEventListener(MouseEvent.MOUSE_UP, function(evt:Event)
												 {
													 var gameID:int = evt.currentTarget.parent.parent.gameID;
													 FrontEndEvents.eventBrowseGamesOnAddToFavorites(gameID);
													 evt.currentTarget.parent.visible = false;
													 evt.currentTarget.parent.parent.BtnRem.visible = true;
												 });
				slot.BtnRem.Btn.addEventListener(MouseEvent.MOUSE_OVER, function(evt:Event)
												 {
													 evt.currentTarget.parent.gotoAndPlay("over");
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.MOUSE_OVER));
												 });
				slot.BtnRem.Btn.addEventListener(MouseEvent.MOUSE_OUT, function(evt:Event)
												 {
													 evt.currentTarget.parent.gotoAndPlay("out");
													 evt.currentTarget.parent.parent.Btn.dispatchEvent(new Event(MouseEvent.MOUSE_OUT));
												 });
				slot.BtnRem.Btn.addEventListener(MouseEvent.MOUSE_DOWN, function(evt:Event)
												 {
													 SoundEvents.eventSoundPlay("menu_click");
													 evt.currentTarget.parent.gotoAndPlay("pressed");
												 });
				slot.BtnRem.Btn.addEventListener(MouseEvent.MOUSE_UP, function(evt:Event)
												 {
													 var gameID:int = evt.currentTarget.parent.parent.gameID;
													 FrontEndEvents.eventBrowseGamesOnAddToFavorites(gameID);
													 evt.currentTarget.parent.visible = false;
													 evt.currentTarget.parent.parent.BtnFav.visible = true;
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
												  for(var i=0; i<me.GameList.length; ++i)
												  {
													  if(me.GameList[i]["movie"].State == "active")
													  {
														  me.GameList[i]["movie"].State = "none";
														  me.GameList[i]["movie"].gotoAndPlay("out");
													  }
												  }
												  SoundEvents.eventSoundPlay("menu_click");
												  evt.currentTarget.parent.State = "active";
												  evt.currentTarget.parent.gotoAndPlay("pressed");
												  ServBrowse.BtnJoinServer.visible = true;
												  ServBrowse.BtnDonateGC.visible = evt.currentTarget.parent.DonateAllowed;
												  SelectedGameID = evt.currentTarget.parent.gameID;
											  }
											  // double click functionality
											  if(evt.currentTarget.timeOutTimer == undefined)
											  {
												  evt.currentTarget.pressCounter = 0;
												  evt.currentTarget.timeOutTimer = null;
												  evt.currentTarget.timeOut = function() { this.pressCounter = 0; clearTimeout(this.timeOutTimer); }
											  }
											  evt.currentTarget.pressCounter++;
											  if(evt.currentTarget.pressCounter==1)
											  	  evt.currentTarget.timeOutTimer = setTimeout(evt.currentTarget.timeOut, 250);
											  else
											  {
												  evt.currentTarget.timeOut();
												  // double click
												  FrontEndEvents.eventBrowseGamesJoin(SelectedGameID);
											  }											
										  });
				
				ServBrowse.List.addChild(slot);				
			}
			
			if (GameList.length > 13)
			{
				Scroller.visible = true;
				
				Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);
				
				Scroller.ArrowDown.y = ServBrowse.Mask.height - 4;
				Scroller.Field.height = (ServBrowse.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				ServBrowse.List.y = 293;
			}
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = ServBrowse.List.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = ServBrowse.Mask.height;
			h -= mh;
			h /= h1;

			var	dest:Number = dist * h;
			var	step:Number = int (dest / 47);
			
			if (ServBrowse.List.y != (293+(47 * step)))
			{
				Tweener.addTween(ServBrowse.List, {y:(293+(47 * step)), time:api.tweenDelay, transition:"linear"});
			}
		}


		public	function mouseUp(e:Event) :void
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > ServBrowse.Mask.x && 
				e.stageX < ServBrowse.Mask.x + ServBrowse.Mask.width + 45 &&
				e.stageY > ServBrowse.Mask.y && 
				e.stageY < ServBrowse.Mask.y + ServBrowse.Mask.height)
			{
				var dist = (ServBrowse.List.height - ServBrowse.Mask.height) / 47;
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

	}
}