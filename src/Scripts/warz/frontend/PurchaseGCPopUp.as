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
	
	public class PurchaseGCPopUp extends MovieClip {
		
		public var BlockMoney:MovieClip;
		public var BlockGC:MovieClip;
		public var Bar:MovieClip;
		public var BtnCancel:MovieClip;
		public var BtnBuy:MovieClip;
		
		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;
		
		private var isGCSelling:Boolean;

		public var api:warz.frontend.Frontend=null;
		
		private var currentPrice:int = 0;

		public	function PurchaseGCPopUp ()
		{
			visible = false;
			
			BtnBuy.Text.Text.text = "$FR_Buy";
			BtnCancel.Text.Text.text = "$FR_Cancel";
			
			BtnBuy.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			BtnBuy.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			BtnBuy.Btn.addEventListener(MouseEvent.CLICK, MouseBuyClick);

			BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			BtnCancel.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			BtnCancel.Btn.addEventListener(MouseEvent.CLICK, MouseCancelClick);
			
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
			Scroller.ArrowLeft.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown(-1);})
			Scroller.ArrowRight.addEventListener(MouseEvent.MOUSE_OVER, function() {me.Scroller.ArrowRight.alpha = 1;})
			Scroller.ArrowRight.addEventListener(MouseEvent.MOUSE_OUT, function() {me.Scroller.ArrowRight.alpha = 0.5; })
			Scroller.ArrowRight.addEventListener(MouseEvent.CLICK, function() {me.handleScrollerUpDown(+1);})
			
			Scroller.Trigger.addEventListener(MouseEvent.MOUSE_DOWN, function() 
			{
				var startX = 10;
				var endX = 750;
				me.ScrollerIsDragging = true;
				me.Scroller.Trigger.startDrag(false, new Rectangle (startX, 52, endX, 0));
			})
		}
		
		public function showPopUp(isGC:Boolean)
		{
			api = warz.frontend.Frontend.api;
			isGCSelling = isGC;
			
			if(isGCSelling)
				this.Title.text = "$FR_CREATE_CHARACTER_PURCHASE_GC";
			else
				this.Title.text = "$FR_CONVERT_GC_TO_GD";
				
			if(isGCSelling)
				BlockGC.Value.IconGC.gotoAndStop(1);
			else
				BlockGC.Value.IconGC.gotoAndStop(2);
			
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
			if(isGCSelling)
				FrontEndEvents.eventStorePurchaseGP(currentPrice);
			else
				FrontEndEvents.eventStorePurchaseGD(currentPrice);
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
			BlockGC.Value.Value.autoSize = TextFieldAutoSize.CENTER;
			BlockGC.Value.Value.text = val;
			BlockGC.Value.IconGC.x = BlockGC.Value.Value.x + BlockGC.Value.Value.width;
		}
		
		public	function scrollItems(e:Event)
		{
			Bar.Bar.width = Scroller.Trigger.x-Bar.Bar.x + 5;

			var currentXValue = Scroller.Trigger.x-10;
			
			if(isGCSelling)
			{
				currentPrice = 500+(currentXValue/750)*9500; // 750 - max trigger movement, 10000 - max price
				currentPrice = Math.min(Math.max(500, currentPrice), 10000);
				
				var wholeNumber:int = int(Number(currentPrice)/100);
				var partNumber:int = currentPrice-(wholeNumber*100);
				
				// part number should be rounded to 20 cents
				partNumber = int(partNumber/20)*20;
				
				currentPrice = wholeNumber*100+partNumber;

				if(partNumber < 10)
					BlockMoney.Value.text = Bar.Value.text = "$"+wholeNumber+".0"+partNumber;
				else
					BlockMoney.Value.text = Bar.Value.text = "$"+wholeNumber+"."+partNumber;

				// call callback
				if(api.isDebug)
				{
					var conversionString = "$1 = 333GC";
					Bar.Rate.text = conversionString;
					setGCValue(6633);
				}
				else
				{
					FrontEndEvents.eventStorePurchaseGPCallback(currentPrice);
				}
			}
			else
			{
				currentPrice = 10+(currentXValue/750)*2990; // 750 - max trigger movement, 3000 - max price
				currentPrice = Math.min(Math.max(10, currentPrice), 3000);
				
				BlockMoney.Value.text = Bar.Value.text = currentPrice+" GC";

				// call callback
				if(api.isDebug)
				{
					Bar.Rate.text = "1GC = 100GD";
					setGCValue(6633);
				}
				else
				{
					FrontEndEvents.eventStorePurchaseGDCallback(currentPrice);
				}
			}
		}
	}
}