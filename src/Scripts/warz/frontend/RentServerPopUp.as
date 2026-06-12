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
	import flash.geom.Point;
	import warz.Utils;

	public class RentServerPopUp extends MovieClip {
		public var isActiveScreen:Boolean = false;
		
		public var api:warz.frontend.Frontend=null;
		
		private var isShowNameplates:int = 0;
		private var isShowTracers:int = 0;
		private var isShowCrosshair:int = 0;
		private var isPVEServer:int = 0;
		
		public var isGameServerRenting:int = 0;
		public var renewServerID:uint = 0;
		
		public var buttons:Array = new Array();
		
		private var currentRegionID:int =0;
		private var currentSlotID:int =0;
		private var currentPeriodID:int =0;
		private var currentMapID:int =0;
		
		public	function RentServerPopUp ()
		{
			visible = false;
			var me = this;
			
			BtnRent.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			BtnRent.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			BtnRent.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			BtnRent.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			BtnBack.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			BtnBack.Text.Text.text = "$FR_Back";

			BtnRent.ActionFunction = function()
			{
				ServerName.Value.text = warz.Utils.trimWhitespace(ServerName.Value.text);
				if(isGameServerRenting == 2)
					FrontEndEvents.eventRenewServer(renewServerID, currentPeriodID);
				else
					FrontEndEvents.eventRentServer(isGameServerRenting, currentMapID, currentRegionID, currentSlotID, currentPeriodID, 
													  ServerName.Value.text, Password.Value.text, isPVEServer, 
													  isShowNameplates, isShowCrosshair, isShowTracers);
			}
			BtnBack.ActionFunction = function()
			{
				api.Main.showScreen("MyServers");
			}
			
			CheckboxText1.text = "$FR_ShowNameplates";
			CheckboxText2.text = "$FR_ShowCrosshair";
			CheckboxText3.text = "$FR_ShowTracers";

			Checkbox1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			Checkbox1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			Checkbox1.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			Checkbox1.ActionFunction = function(param:int)
			{
				isShowNameplates = param;
				updatePriceRequest();
			}
			Checkbox2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			Checkbox2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			Checkbox2.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			Checkbox2.ActionFunction = function(param:int)
			{
				isShowCrosshair = param;
				updatePriceRequest();
			}
			Checkbox3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn2);
			Checkbox3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn2);
			Checkbox3.Btn.addEventListener(MouseEvent.CLICK, BtnPressFn2);
			Checkbox3.ActionFunction = function(param:int)
			{
				isShowTracers = param;
				updatePriceRequest();
			}
			
			Map.Name.text = "$FR_MapType";
			Map.Value.text = "";
			Map.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Map.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Map.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			Map.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			Map.ArrowLeft.ActionFunction = function()
			{
				//todo
			}
			Map.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn);
			Map.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn);
			Map.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn);
			Map.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn);
			Map.ArrowRight.ActionFunction = function()
			{
			}

			BlockRegion.Title.text = "$FR_REGION";
			BlockRegion.EU.groupID = 1;
			BlockRegion.US.groupID = 1;
			BlockRegion.SA.groupID = 1;
			BlockRegion.RU.groupID = 1;
			BlockRegion.US.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockRegion.US.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockRegion.US.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockRegion.US.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockRegion.US.ActionFunction = function()
			{
				deselectButtons(this);
				currentRegionID = this.valueID;
				updatePriceRequest();
			}
			BlockRegion.EU.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockRegion.EU.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockRegion.EU.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockRegion.EU.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockRegion.EU.ActionFunction = function()
			{
				deselectButtons(this);
				currentRegionID = this.valueID;
				updatePriceRequest();
			}
			BlockRegion.SA.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockRegion.SA.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockRegion.SA.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockRegion.SA.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockRegion.SA.ActionFunction = function()
			{
				deselectButtons(this);
				currentRegionID = this.valueID;
				updatePriceRequest();
			}
			BlockRegion.RU.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockRegion.RU.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockRegion.RU.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockRegion.RU.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockRegion.RU.ActionFunction = function()
			{
				deselectButtons(this);
				currentRegionID = this.valueID;
				updatePriceRequest();
			}
			
			buttons.push(BlockRegion.EU);
			buttons.push(BlockRegion.US);
			buttons.push(BlockRegion.SA);
			buttons.push(BlockRegion.RU);

			BlockSlots.Title.text = "$FR_Slots";
			BlockSlots.Btn1.groupID = 2;
			BlockSlots.Btn2.groupID = 2;
			BlockSlots.Btn3.groupID = 2;
			BlockSlots.Btn4.groupID = 2;
			BlockSlots.Btn5.groupID = 2;
			
			BlockSlots.Btn1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockSlots.Btn1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockSlots.Btn1.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockSlots.Btn1.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockSlots.Btn1.ActionFunction = function()
			{
				deselectButtons(this);
				currentSlotID = this.valueID;
				updatePriceRequest();
			}
			BlockSlots.Btn2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockSlots.Btn2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockSlots.Btn2.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockSlots.Btn2.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockSlots.Btn2.ActionFunction = function()
			{
				deselectButtons(this);
				currentSlotID = this.valueID;
				updatePriceRequest();
			}
			BlockSlots.Btn3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockSlots.Btn3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockSlots.Btn3.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockSlots.Btn3.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockSlots.Btn3.ActionFunction = function()
			{
				deselectButtons(this);
				currentSlotID = this.valueID;
				updatePriceRequest();
			}
			BlockSlots.Btn4.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockSlots.Btn4.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockSlots.Btn4.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockSlots.Btn4.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockSlots.Btn4.ActionFunction = function()
			{
				deselectButtons(this);
				currentSlotID = this.valueID;
				updatePriceRequest();
			}
			BlockSlots.Btn5.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockSlots.Btn5.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockSlots.Btn5.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockSlots.Btn5.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockSlots.Btn5.ActionFunction = function()
			{
				deselectButtons(this);
				currentSlotID = this.valueID;
				updatePriceRequest();
			}
			buttons.push(BlockSlots.Btn1);
			buttons.push(BlockSlots.Btn2);
			buttons.push(BlockSlots.Btn3);
			buttons.push(BlockSlots.Btn4);
			buttons.push(BlockSlots.Btn5);

			BlockDuration.Title.text = "$FR_RentalPeriod";
			BlockDuration.Btn1.groupID = 3;
			BlockDuration.Btn2.groupID = 3;
			BlockDuration.Btn3.groupID = 3;
			BlockDuration.Btn4.groupID = 3;
			BlockDuration.Btn5.groupID = 3;
			
			BlockDuration.Btn1.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockDuration.Btn1.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockDuration.Btn1.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockDuration.Btn1.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockDuration.Btn1.ActionFunction = function()
			{
				deselectButtons(this);
				currentPeriodID = this.valueID;
				updatePriceRequest();
			}
			BlockDuration.Btn2.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockDuration.Btn2.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockDuration.Btn2.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockDuration.Btn2.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockDuration.Btn2.ActionFunction = function()
			{
				deselectButtons(this);
				currentPeriodID = this.valueID;
				updatePriceRequest();
			}
			BlockDuration.Btn3.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockDuration.Btn3.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockDuration.Btn3.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockDuration.Btn3.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockDuration.Btn3.ActionFunction = function()
			{
				deselectButtons(this);
				currentPeriodID = this.valueID;
				updatePriceRequest();
			}
			BlockDuration.Btn4.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockDuration.Btn4.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockDuration.Btn4.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockDuration.Btn4.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockDuration.Btn4.ActionFunction = function()
			{
				deselectButtons(this);
				currentPeriodID = this.valueID;
				updatePriceRequest();
			}
			BlockDuration.Btn5.Btn.addEventListener(MouseEvent.MOUSE_OVER, BtnRollOverFn3);
			BlockDuration.Btn5.Btn.addEventListener(MouseEvent.MOUSE_OUT, BtnRollOutFn3);
			BlockDuration.Btn5.Btn.addEventListener(MouseEvent.MOUSE_DOWN, BtnPressFn3);
			BlockDuration.Btn5.Btn.addEventListener(MouseEvent.MOUSE_UP, BtnPressUpFn3);
			BlockDuration.Btn5.ActionFunction = function()
			{
				deselectButtons(this);
				currentPeriodID = this.valueID;
				updatePriceRequest();
			}
			
			buttons.push(BlockDuration.Btn1);
			buttons.push(BlockDuration.Btn2);
			buttons.push(BlockDuration.Btn3);
			buttons.push(BlockDuration.Btn4);
			buttons.push(BlockDuration.Btn5);
			
			ServerName.Name.text = "$FR_ServerName";
			Password.Name.text = "$FR_Password";
			
			Password.Value.addEventListener(Event.CHANGE, function(event:Event) {
											updatePriceRequest();
											}); 
			
		}
		
		public function deselectButtons(mv:MovieClip)
		{
			var grpID = mv.groupID;
			for(var i=0; i<buttons.length; ++i)
			{
				if(buttons[i].groupID == grpID && buttons[i]!=mv && buttons[i].currentLabel == "pressed")
				{
					buttons[i].gotoAndPlay("out");
				}
			}
		}
		
		public function updatePriceRequest()
		{
			if(isGameServerRenting==2)
				FrontEndEvents.eventRenewServerUpdatePrice(renewServerID, currentPeriodID);
			else
				FrontEndEvents.eventRentServerUpdatePrice(isGameServerRenting, currentMapID, currentRegionID, currentSlotID, currentPeriodID, 
													  ServerName.Value.text, Password.Value.text, isPVEServer, 
													  isShowNameplates, isShowCrosshair, isShowTracers);
		}
		
		public function updateServerPrice(p:int)
		{
			this.Value.text = String(p);
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

		private function BtnRollOverFn3(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel != "pressed" && evt.currentTarget.parent.currentLabel != "inactive")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		private function BtnRollOutFn3(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel != "pressed" && evt.currentTarget.parent.currentLabel != "inactive")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		private function BtnPressFn3(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel != "pressed" && evt.currentTarget.parent.currentLabel != "inactive")
			{
				SoundEvents.eventSoundPlay("menu_click");
				evt.currentTarget.parent.gotoAndPlay("pressed");
			}
		}
		private function BtnPressUpFn3(evt:Event)
		{
			if(evt.currentTarget.parent.currentLabel == "pressed" && evt.currentTarget.parent.currentLabel != "inactive")
			{
				evt.currentTarget.parent.ActionFunction();
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

		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			
			if(isGameServerRenting==1)
				Title.text = "$FR_RentGameserver";
			else if(isGameServerRenting==0)
				Title.text = "$FR_RentStronghold";
			else if(isGameServerRenting==2)
				Title.text = "$FR_RenewServer";
				
			if(isGameServerRenting != 2)
				BtnRent.Text.Text.text = "$FR_RENT_SERVER";
			else
				BtnRent.Text.Text.text = "$FR_RENEW_SERVER";
			
			if(api.RentServer_Region.length == 1) 
			{
				BlockRegion.EU.visible = false;
				BlockRegion.US.visible = false;
				BlockRegion.SA.visible = false;
				BlockRegion.RU.Text.Text.text = api.RentServer_Region[0]["displayValue"];
				BlockRegion.RU.valueID = api.RentServer_Region[0]["id"];
				BlockRegion.RU.gotoAndPlay("pressed");
				currentRegionID = BlockRegion.RU.valueID;
				deselectButtons(BlockRegion.RU);
			}
			else
			{
				BlockRegion.RU.visible = false;
				BlockRegion.US.Text.Text.text = api.RentServer_Region[0]["displayValue"];
				BlockRegion.US.valueID = api.RentServer_Region[0]["id"];
				BlockRegion.US.FlagBW.gotoAndStop(2);
				BlockRegion.US.Flag.gotoAndStop(2);
				BlockRegion.EU.Text.Text.text = api.RentServer_Region[1]["displayValue"];
				BlockRegion.EU.valueID = api.RentServer_Region[1]["id"];
				BlockRegion.EU.FlagBW.gotoAndStop(1);
				BlockRegion.EU.Flag.gotoAndStop(1);
				BlockRegion.SA.Text.Text.text = api.RentServer_Region[2]["displayValue"];
				BlockRegion.SA.valueID = api.RentServer_Region[2]["id"];
				BlockRegion.SA.FlagBW.gotoAndStop(3);
				BlockRegion.SA.Flag.gotoAndStop(3);
				
				currentRegionID = BlockRegion.US.valueID;
				BlockRegion.US.gotoAndPlay("pressed");
				deselectButtons(BlockRegion.US);
			}
			
			if(isGameServerRenting==2)
			{
				BlockRegion.EU.gotoAndStop("inactive");
				BlockRegion.US.gotoAndStop("inactive");
				BlockRegion.SA.gotoAndStop("inactive");
				BlockRegion.RU.gotoAndStop("inactive");
			}
			else
			{
				BlockRegion.EU.gotoAndStop(1);
				BlockRegion.US.gotoAndStop(1);
				BlockRegion.SA.gotoAndStop(1);
				BlockRegion.RU.gotoAndStop(1);
			}
			
			if(isGameServerRenting>0)
			{
				currentMapID = api.RentServer_Maps[0]["id"];
				Map.Value.text = api.RentServer_Maps[0]["displayValue"];
			}
			else
			{
				currentMapID = api.RentServer_MapsStronghold[0]["id"];
				Map.Value.text = api.RentServer_MapsStronghold[0]["displayValue"];
			}
			
			if(isGameServerRenting>0)
			{
				BlockSlots.Btn1.Text.Text.text = api.RentServer_Slots[0]["displayValue"];
				BlockSlots.Btn1.valueID = api.RentServer_Slots[0]["id"];
				BlockSlots.Btn2.Text.Text.text = api.RentServer_Slots[1]["displayValue"];
				BlockSlots.Btn2.valueID = api.RentServer_Slots[1]["id"];
				BlockSlots.Btn3.Text.Text.text = api.RentServer_Slots[2]["displayValue"];
				BlockSlots.Btn3.valueID = api.RentServer_Slots[2]["id"];
				BlockSlots.Btn4.Text.Text.text = api.RentServer_Slots[3]["displayValue"];
				BlockSlots.Btn4.valueID = api.RentServer_Slots[3]["id"];
				BlockSlots.Btn5.Text.Text.text = api.RentServer_Slots[4]["displayValue"];
				BlockSlots.Btn5.valueID = api.RentServer_Slots[4]["id"];
			}
			else
			{
				BlockSlots.Btn1.Text.Text.text = api.RentServer_SlotsStronghold[0]["displayValue"];
				BlockSlots.Btn1.valueID = api.RentServer_SlotsStronghold[0]["id"];
				BlockSlots.Btn2.Text.Text.text = api.RentServer_SlotsStronghold[1]["displayValue"];
				BlockSlots.Btn2.valueID = api.RentServer_SlotsStronghold[1]["id"];
				BlockSlots.Btn3.Text.Text.text = api.RentServer_SlotsStronghold[2]["displayValue"];
				BlockSlots.Btn3.valueID = api.RentServer_SlotsStronghold[2]["id"];
				BlockSlots.Btn4.Text.Text.text = api.RentServer_SlotsStronghold[3]["displayValue"];
				BlockSlots.Btn4.valueID = api.RentServer_SlotsStronghold[3]["id"];
				BlockSlots.Btn5.Text.Text.text = api.RentServer_SlotsStronghold[4]["displayValue"];
				BlockSlots.Btn5.valueID = api.RentServer_SlotsStronghold[4]["id"];
			}

			if(isGameServerRenting==2)
			{
				BlockSlots.Btn1.gotoAndStop("inactive");
				BlockSlots.Btn2.gotoAndStop("inactive");
				BlockSlots.Btn3.gotoAndStop("inactive");
				BlockSlots.Btn4.gotoAndStop("inactive");
				BlockSlots.Btn5.gotoAndStop("inactive");
			}			
			else
			{
				if(isGameServerRenting>0)
				{
					if(!api.RentServer_Slots[0]["enabled"])
						BlockSlots.Btn1.gotoAndStop("inactive");
					else
						BlockSlots.Btn1.gotoAndStop(1);
					if(!api.RentServer_Slots[1]["enabled"])
						BlockSlots.Btn2.gotoAndStop("inactive");
					else
						BlockSlots.Btn2.gotoAndStop(1);
					if(!api.RentServer_Slots[2]["enabled"])
						BlockSlots.Btn3.gotoAndStop("inactive");
					else
						BlockSlots.Btn3.gotoAndStop(1);
					if(!api.RentServer_Slots[3]["enabled"])
						BlockSlots.Btn4.gotoAndStop("inactive");
					else
						BlockSlots.Btn4.gotoAndStop(1);
					if(!api.RentServer_Slots[4]["enabled"])
						BlockSlots.Btn5.gotoAndStop("inactive");
					else
						BlockSlots.Btn5.gotoAndStop(1);
				}
				else
				{
					if(!api.RentServer_SlotsStronghold[0]["enabled"])
						BlockSlots.Btn1.gotoAndStop("inactive");
					else
						BlockSlots.Btn1.gotoAndStop(1);
					if(!api.RentServer_SlotsStronghold[1]["enabled"])
						BlockSlots.Btn2.gotoAndStop("inactive");
					else
						BlockSlots.Btn2.gotoAndStop(1);
					if(!api.RentServer_SlotsStronghold[2]["enabled"])
						BlockSlots.Btn3.gotoAndStop("inactive");
					else
						BlockSlots.Btn3.gotoAndStop(1);
					if(!api.RentServer_SlotsStronghold[3]["enabled"])
						BlockSlots.Btn4.gotoAndStop("inactive");
					else
						BlockSlots.Btn4.gotoAndStop(1);
					if(!api.RentServer_SlotsStronghold[4]["enabled"])
						BlockSlots.Btn5.gotoAndStop("inactive");
					else
						BlockSlots.Btn5.gotoAndStop(1);
				}
				currentSlotID = BlockSlots.Btn2.valueID;
				BlockSlots.Btn2.gotoAndPlay("pressed");
				deselectButtons(BlockSlots.Btn2);
			}
			
			BlockDuration.Btn1.Text.TextNum.text = api.RentServer_Rental[0]["num"];
			BlockDuration.Btn1.Text.Text.text = api.RentServer_Rental[0]["displayValue"];
			BlockDuration.Btn1.valueID = api.RentServer_Rental[0]["id"];
			BlockDuration.Btn2.Text.TextNum.text = api.RentServer_Rental[1]["num"];
			BlockDuration.Btn2.Text.Text.text = api.RentServer_Rental[1]["displayValue"];
			BlockDuration.Btn2.valueID = api.RentServer_Rental[1]["id"];
			BlockDuration.Btn3.Text.TextNum.text = api.RentServer_Rental[2]["num"];
			BlockDuration.Btn3.Text.Text.text = api.RentServer_Rental[2]["displayValue"];
			BlockDuration.Btn3.valueID = api.RentServer_Rental[2]["id"];
			BlockDuration.Btn4.Text.TextNum.text = api.RentServer_Rental[3]["num"];
			BlockDuration.Btn4.Text.Text.text = api.RentServer_Rental[3]["displayValue"];
			BlockDuration.Btn4.valueID = api.RentServer_Rental[3]["id"];
			BlockDuration.Btn5.Text.TextNum.text = api.RentServer_Rental[4]["num"];
			BlockDuration.Btn5.Text.Text.text = api.RentServer_Rental[4]["displayValue"];
			BlockDuration.Btn5.valueID = api.RentServer_Rental[4]["id"];
			
			currentPeriodID = BlockDuration.Btn1.valueID;
			BlockDuration.Btn1.gotoAndPlay("pressed");
			deselectButtons(BlockDuration.Btn1);
			
			this.Value.text = "0";

			ServerName.Value.text = "";
			Password.Value.text = "";
			
			if(isGameServerRenting==2)
			{
				Password.gotoAndStop("off");
				ServerName.gotoAndStop("off");
				Map.gotoAndStop("off");
			}
			else
			{
				Password.gotoAndStop("on");
				ServerName.gotoAndStop("on");
				Map.gotoAndStop("on");
			}

			this.visible = true;

			updatePriceRequest();
		}
		
		public function Deactivate()
		{
			this.visible = false;
		}
	}
}