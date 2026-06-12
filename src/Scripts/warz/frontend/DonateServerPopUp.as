package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.*;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	import caurina.transitions.Tweener;
	import flash.text.TextFieldAutoSize;
	
	public class DonateServerPopUp extends MovieClip {
		
		public var Bar:MovieClip;
		public var BtnCancel:MovieClip;
		public var BtnBuy:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;
		
		public var api:warz.frontend.Frontend=null;
		
		private var currentHours:int = 0;
		private var selectedServerID:int = 0;

		public	function DonateServerPopUp ()
		{
			visible = false;
			
			BtnBuy.Text.Text.text = "$FR_ClanDonateGC";
			BtnCancel.Text.Text.text = "$FR_Cancel";
			this.Title.text = "$FR_DonateGCToServer";
			
			BtnBuy.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			BtnBuy.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			BtnBuy.Btn.addEventListener(MouseEvent.CLICK, MouseBuyClick);

			BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			BtnCancel.Btn.addEventListener(MouseEvent.CLICK, MouseCancelClick);
			
			BlockSpend.Title.text = "$FR_ClanPlDG";
			BlockGet.Title.text = "$FR_ServerDonatedHours";
			Bar.Title.text = "$FR_SelectAmountOfGCToDonate";
			
			var me = this;
			Scroller = Bar;
			ScrollerIsDragging = false;
			Scroller.ArrowLeft.alpha = 0.5;
			Scroller.ArrowRight.alpha = 0.5;
			Scroller.Trigger.alpha = 0.5;

			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_OVER, function() {if(me.ScrollerIsDragging==true) return; me.Scroller.Trigger.alpha = 1; })
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_OUT, function() {if(me.ScrollerIsDragging==true) return; me.Scroller.Trigger.alpha = 0.5; })
			Scroller.ArrowLeft.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller.ArrowLeft.alpha = 1;})
			Scroller.ArrowLeft.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller.ArrowLeft.alpha = 0.5;})
			Scroller.ArrowLeft.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown(-4);})
			Scroller.ArrowRight.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller.ArrowRight.alpha = 1;})
			Scroller.ArrowRight.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller.ArrowRight.alpha = 0.5; })
			Scroller.ArrowRight.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown(+4);})
			
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function() 
			{
				var startX = 10;
				var endX = 750;
				me.ScrollerIsDragging = true;
				me.Scroller.Trigger.startDrag(false, new Rectangle (startX, 52, endX, 0));
			})
		}
		
		public function showPopUp(serverID:int)
		{
			api = warz.frontend.Frontend.api;
			selectedServerID = serverID;
			
			BlockSpend.Value.IconGC.gotoAndStop(1);
			
			Scroller.Trigger.x = 10;

			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
			Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);

			visible = true;
		}
		
		public function hidePopUp()
		{
			Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);
			visible = false;
		}
		
		public	function MouseOver (evt:MouseEvent):void
		{
			if (evt.currentTarget.parent.currentLabel != "over")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function MouseOut (evt:MouseEvent):void
		{
			if (evt.currentTarget.parent.currentLabel != "out")
				evt.currentTarget.parent.gotoAndPlay("out");
		}

		public	function MouseBuyClick (evt:MouseEvent):void
		{
			hidePopUp();			
			SoundEvents.eventSoundPlay("menu_click");
			FrontEndEvents.eventDonateGCtoServer(currentHours, selectedServerID);
		}

		public	function MouseCancelClick (evt:MouseEvent):void
		{
			hidePopUp();
			SoundEvents.eventSoundPlay("menu_click");
		}
		
		public	function mouseUp(e:Event) :void
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
		}

		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > Bar.x && 
				e.stageX < Bar.x + Bar.width &&
				e.stageY > Bar.y && 
				e.stageY < Bar.y + Bar.height)
			{
				var dist = 10;
				
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
				var startX = 10; 
				if(Scroller.Trigger.x > startX) { 
					Scroller.Trigger.x += delta; 
					if(Scroller.Trigger.x < startX) 
						Scroller.Trigger.x = startX; 
				} 
			}
			else
			{
				var endX = 750;
				if(Scroller.Trigger.x < endX) { 
					Scroller.Trigger.x += delta; 
					if(Scroller.Trigger.x > endX) 
						Scroller.Trigger.x = endX; 
				} 
			}
			Bar.Bar.width = Scroller.Trigger.x-Bar.Bar.x + 5;
		}
		
		public function setGCValue(val:int)
		{
			BlockSpend.Value.Value.autoSize = TextFieldAutoSize.CENTER;
			BlockSpend.Value.Value.text = val;
			BlockSpend.Value.IconGC.x = BlockSpend.Value.Value.x + BlockSpend.Value.Value.width;
		}
		
		public	function scrollItems(e:Event)
		{
			Bar.Bar.width = Scroller.Trigger.x-Bar.Bar.x + 5;

			var currentXValue = Scroller.Trigger.x-10;
			
			{
				currentHours = 1+(currentXValue/750)*199; // 750 - max trigger movement, 200 - max hours
				currentHours = Math.min(Math.max(1, currentHours), 200);
				
				BlockGet.Value.text = Bar.Value.text = currentHours+" $FR_PAUSE_INVENTORY_HR";

				// call callback
				if(api.isDebug)
				{
					setGCValue(1500);
				}
				else
				{
					FrontEndEvents.eventDonateGCtoServerCallback(currentHours, selectedServerID);
				}
			}
		}
	}
}