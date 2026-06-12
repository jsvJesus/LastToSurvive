package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.utils.clearInterval;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.utils.setInterval;
	import flash.geom.Point;
	import caurina.transitions.Tweener;
	
	public class OptionsControl extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public	var	OptionsControlsClip:MovieClip;

		public var api:warz.frontend.Frontend=null;

		public var Opt_vertLook :Object;
		public var Opt_mouseWheel :Object;
		public var Opt_mouseSens :Object;
		public var Opt_mouseAccel :Object;
		public var Opt_toggleAim :Object;
		public var Opt_toggleCrouch :Object;
	
		public var arNY :Array; // No, Yes
		public var arPT :Array; // Press, Toggle

		private var listenForKeyboardTarget : MovieClip;
		private var sendKeyRemapRequestInterval;
		
		public	var	keyControls:MovieClip;

		private var ScrollerIsDragging:Boolean;
		private var Scroller:MovieClip;

		public	var	isMouseDown:Boolean;
		
		public	function OptionsControl ()
		{
			OptionsControlsClip.General_Options.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsControlsClip.General_Options.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsControlsClip.General_Options.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsControlsClip.Controls.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsControlsClip.Controls.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsControlsClip.Controls.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsControlsClip.Language.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsControlsClip.Language.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsControlsClip.Language.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			OptionsControlsClip.Voip.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsControlsClip.Voip.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsControlsClip.Voip.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsControlsClip.BtnReset.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsControlsClip.BtnReset.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsControlsClip.BtnReset.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);
			
			OptionsControlsClip.BtnApply.Btn.addEventListener(MouseEvent.MOUSE_OVER, SlotRollOverFn);
			OptionsControlsClip.BtnApply.Btn.addEventListener(MouseEvent.MOUSE_OUT, SlotRollOutFn);
			OptionsControlsClip.BtnApply.Btn.addEventListener(MouseEvent.CLICK, SlotPressFn);

			OptionsControlsClip.BtnReset.Text.Text.text = "$FR_Reset";
			OptionsControlsClip.BtnApply.Text.Text.text = "$FR_Apply";
			
			Opt_vertLook = new Object();
			Opt_mouseWheel = new Object();
			Opt_mouseSens = new Object();
			Opt_mouseAccel = new Object();
			Opt_toggleAim = new Object();
			Opt_toggleCrouch = new Object();
			
			arNY = new Array("$FR_optNo", "$FR_optYes"); 
			arPT = new Array("$FR_optPress", "$FR_optToggle");
			
			keyControls = new MovieClip ();
			keyControls.mask = OptionsControlsClip.Mask;				
			OptionsControlsClip.addChild(keyControls);
			
			var	me = this;
			
			Scroller = OptionsControlsClip.Scroller;
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
			
		}
		
		public function Activate()
		{
			api = warz.frontend.Frontend.api;
			
			this.gotoAndPlay("start");
			
			resetValues();

			OptionsControlsClip.Controls.State = "active";
			if (OptionsControlsClip.Controls.currentLable != "pressed")
				OptionsControlsClip.Controls.gotoAndPlay("pressed");

			api.setButtonText (OptionsControlsClip.General_Options.Text, "$FR_optGeneral");
			api.setButtonText (OptionsControlsClip.Controls.Text, "$FR_optControls");
			api.setButtonText (OptionsControlsClip.Language.Text, "$FR_optLanguage");
			api.setButtonText (OptionsControlsClip.Voip.Text, "$FR_optVoip");
			
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.addEventListener(MouseEvent.MOUSE_UP, mouseUp);
		}
		
		public function Deactivate()
		{
			stage.removeEventListener(MouseEvent.MOUSE_WHEEL, scrollerMouseWheel);
			stage.removeEventListener(MouseEvent.MOUSE_UP, mouseUp);
			
			this.gotoAndPlay("end");
		}
		
		public	function mouseUp(e:Event) :void
		{ 
			ScrollerIsDragging = false;
			Scroller.Trigger.stopDrag(); 
		}
		
		public	function scrollerMouseWheel (e:MouseEvent):void
		{
			if (e.stageX > OptionsControlsClip.Mask.x && 
				e.stageX < OptionsControlsClip.Mask.x + OptionsControlsClip.Mask.width + 45 &&
				e.stageY > OptionsControlsClip.Mask.y && 
				e.stageY < OptionsControlsClip.Mask.y + OptionsControlsClip.Mask.height)
			{
				var dist = (keyControls.height - OptionsControlsClip.Mask.height) / 51;
				var h = Scroller.Field.height - Scroller.Trigger.height + 26;
				dist = h / dist;

				if (e.delta > 0)
					handleScrollerUpDown(-dist);
				else
					handleScrollerUpDown(dist);
			}
		}

		private function SlotRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		private function SlotRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		private function SlotPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("pressed");
				
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as OptionsControl))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as OptionsControl).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public function ActionFunction (button:String)
		{
			if (button == "General_Options")
			{
				api.showOptionsGeneral();
				
				OptionsControlsClip.General_Options.State = "off";
				OptionsControlsClip.General_Options.gotoAndPlay("out");
			}
			else if (button == "Controls")
			{
//				api.showOptionsControls();
				OptionsControlsClip.Controls.State = "off";
				OptionsControlsClip.Controls.gotoAndPlay("out");
			}
			else if (button == "Language")
			{
				api.showOptionsLanguage();

				OptionsControlsClip.Language.State = "off";
				OptionsControlsClip.Language.gotoAndPlay("out");
			}
			else if (button == "Voip")
			{
				api.showOptionsVoip();

				OptionsControlsClip.Voip.State = "off";
				OptionsControlsClip.Voip.gotoAndPlay("out");
			}
			else if (button == "BtnApply")
			{ 
				OptionsControlsClip.BtnApply.State = "off";
				OptionsControlsClip.BtnApply.gotoAndPlay("out");
				
				api.showInfoOkCancelMsg("$FR_ApplySettingAreYouSure", "",function(isApply:Boolean){
						if(isApply){
							Opt_mouseSens.myValue = getSliderValue2(OptionsControlsClip.OptStat4);
							FrontEndEvents.eventOptionsControlsApply(Opt_vertLook.myValue, 0, Opt_mouseWheel.myValue, 
															 Opt_mouseSens.myValue, Opt_mouseAccel.myValue, 
															 Opt_toggleAim.myValue, Opt_toggleCrouch.myValue);
						}
				});
			}
			else if (button == "BtnReset")
			{
				OptionsControlsClip.BtnReset.State = "off";
				OptionsControlsClip.BtnReset.gotoAndPlay("out");
				
				api.showInfoOkCancelMsg("$FR_ResetSettingAreYouSure", "",function(isReset:Boolean){
										if(isReset)FrontEndEvents.eventOptionsControlsReset();});
			}
		}
		
		public	function resetValues ()
		{
			var	me = this;
			api = warz.frontend.Frontend.api;

			Opt_vertLook.myValue = api.Opt_vertLook;
			Opt_mouseWheel.myValue = api.Opt_mouseWheel;
			Opt_mouseSens.myValue = api.Opt_mouseSens;
			Opt_mouseAccel.myValue = api.Opt_mouseAccel;
			Opt_toggleAim.myValue = api.Opt_toggleAim;
			Opt_toggleCrouch.myValue = api.Opt_toggleCrouch;
			
			OptionsControlsClip.OptStat1.Name.text = "$FR_optInvertYAxis";
			OptionsControlsClip.OptStat1.Value.Text.text = arNY[Opt_vertLook.myValue];
//			OptionsControlsClip.OptStat1.Value.gotoAndPlay("start");
			OptionsControlsClip.OptStat1.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_vertLook, me.arNY, this); }
			OptionsControlsClip.OptStat1.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_vertLook, me.arNY, this); }
			OptionsControlsClip.OptStat1.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat1.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat1.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsControlsClip.OptStat1.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat1.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat1.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsControlsClip.OptStat2.Name.text = "$FR_optMouseWheel";
			OptionsControlsClip.OptStat2.Value.Text.text = arNY[Opt_mouseWheel.myValue];
//			OptionsControlsClip.OptStat2.Value.gotoAndPlay("start");
			OptionsControlsClip.OptStat2.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_mouseWheel, me.arNY, this); }
			OptionsControlsClip.OptStat2.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_mouseWheel, me.arNY, this); }
			OptionsControlsClip.OptStat2.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat2.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat2.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsControlsClip.OptStat2.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat2.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat2.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsControlsClip.OptStat3.visible = false;

			OptionsControlsClip.OptStat4.Name.text = "$FR_optSensitivity";
			OptionsControlsClip.OptStat4.Value.Text.text = String (Opt_mouseSens.myValue);
			OptionsControlsClip.OptStat4.ArrowLeft.ActionFunction = function() 
			{ 
				var	val = getSliderValue2 (OptionsControlsClip.OptStat4);
				setSliderValue2(OptionsControlsClip.OptStat4, val - 0.5);
			}
			OptionsControlsClip.OptStat4.ArrowRight.ActionFunction = function() 
			{ 
				var	val = getSliderValue2 (OptionsControlsClip.OptStat4);
				setSliderValue2(OptionsControlsClip.OptStat4, val + 0.5);
			}
			OptionsControlsClip.OptStat4.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat4.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat4.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsControlsClip.OptStat4.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat4.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat4.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsControlsClip.OptStat4.Bar.addEventListener(MouseEvent.MOUSE_DOWN, barPressFn);
			OptionsControlsClip.OptStat4.Bar.addEventListener(MouseEvent.MOUSE_MOVE, barMoveFn);
			OptionsControlsClip.OptStat4.Bar.addEventListener(MouseEvent.MOUSE_UP, barUpFn);
			
			stage.addEventListener(MouseEvent.MOUSE_UP, barUpFn);
			
			OptionsControlsClip.OptStat5.Name.text = "$FR_optAcceleration";
			OptionsControlsClip.OptStat5.Value.Text.text = arNY[Opt_mouseAccel.myValue];
//			OptionsControlsClip.OptStat5.Value.gotoAndPlay("start");
			OptionsControlsClip.OptStat5.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_mouseAccel, me.arNY, this); }
			OptionsControlsClip.OptStat5.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_mouseAccel, me.arNY, this); }
			OptionsControlsClip.OptStat5.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat5.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat5.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsControlsClip.OptStat5.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat5.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat5.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsControlsClip.OptStat6.Name.text = "$FR_optToggleAim";
			OptionsControlsClip.OptStat6.Value.Text.text = arPT[Opt_toggleAim.myValue];
//			OptionsControlsClip.OptStat6.Value.gotoAndPlay("start");
			OptionsControlsClip.OptStat6.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_toggleAim, me.arPT, this); }
			OptionsControlsClip.OptStat6.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_toggleAim, me.arPT, this); }
			OptionsControlsClip.OptStat6.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat6.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat6.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsControlsClip.OptStat6.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat6.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat6.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);

			OptionsControlsClip.OptStat7.Name.text = "$FR_optToggleCrouch";
			OptionsControlsClip.OptStat7.Value.Text.text = arPT[Opt_toggleCrouch.myValue];
//			OptionsControlsClip.OptStat7.Value.gotoAndPlay("start");
			OptionsControlsClip.OptStat7.ArrowLeft.ActionFunction = function() { me.onArrowClick(-1, me.Opt_toggleCrouch, me.arPT, this); }
			OptionsControlsClip.OptStat7.ArrowRight.ActionFunction = function() { me.onArrowClick(+1, me.Opt_toggleCrouch, me.arPT, this); }
			OptionsControlsClip.OptStat7.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat7.ArrowLeft.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat7.ArrowLeft.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			OptionsControlsClip.OptStat7.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OVER, arrowRollOverFn);
			OptionsControlsClip.OptStat7.ArrowRight.Btn.addEventListener(MouseEvent.MOUSE_OUT, arrowRollOutFn);
			OptionsControlsClip.OptStat7.ArrowRight.Btn.addEventListener(MouseEvent.CLICK, arrowPressFn);
			
			setSliderValue2(OptionsControlsClip.OptStat4, Opt_mouseSens.myValue);
			
			updateKeyMapping ();
		}
		
		public	function updateKeyMapping ()
		{
			var me = this;
			listenForKeyboardTarget = null;
//			KeyboardControls.BaseControl._visible = false;
			
			for(var i=0;i<api.keyboardMapping.length;i++)
			{
				var NewName = "ControlStat"+i;
				
				if(OptionsControlsClip[NewName])
					OptionsControlsClip[NewName].visible = false;
			}
			
			while (keyControls.numChildren > 0)
			{
				keyControls.removeChildAt(0);
			}
			
			for(i=0;i<api.keyboardMapping.length;i++)
			{
				var keyControl = new warz.frontend.KeyControl();
				keyControls.addChild(keyControl);
				
				keyControl.x = OptionsControlsClip["ControlStat1"].x;
				keyControl.y = OptionsControlsClip["ControlStat1"].y + 51 * i;
				
				keyControl.addEventListener(MouseEvent.MOUSE_OVER, function (evt:MouseEvent) {
					if (me.listenForKeyboardTarget == null) {
						evt.currentTarget.gotoAndPlay("over");
					}
				});
				keyControl.addEventListener(MouseEvent.MOUSE_OUT, function (evt:MouseEvent) {
					if (me.listenForKeyboardTarget == null) {
						evt.currentTarget.gotoAndPlay("out");
					}
				});
				keyControl.addEventListener(MouseEvent.CLICK, function (evt:MouseEvent) {
					if (me.listenForKeyboardTarget == null) {
						me.listenForKeyboardTarget = evt.currentTarget;
						evt.currentTarget.gotoAndPlay("pressed");
						clearInterval(me.sendKeyRemapRequestInterval);
						me.sendKeyRemapRequestInterval = setInterval (sendKeyRemapRequest, 100);
					}
				});

				keyControl.Name.Name.text = api.keyboardMapping[i]["name"];
				keyControl.Value.Text.text = api.keyboardMapping[i]["key"];
				keyControl.remapIndex = i;
			}
			
			if (api.keyboardMapping.length > 11)
			{
				Scroller.visible = true;
				
				//Scroller.Trigger.y = Scroller.Field.y;
				Scroller.Trigger.addEventListener(Event.ENTER_FRAME, scrollItems);

				Scroller.ArrowDown.y = OptionsControlsClip.Mask.height - 4;
				Scroller.Field.height = (OptionsControlsClip.Mask.height - (Scroller.ArrowDown.height * 2));
			}
			else
			{
				Scroller.Trigger.removeEventListener(Event.ENTER_FRAME, scrollItems);
				Scroller.visible = false;
				keyControls.y = 0;
			}
		}

		public function sendKeyRemapRequest()
		{
			clearInterval(sendKeyRemapRequestInterval);
			FrontEndEvents.eventOptionsControlsRequestKeyRemap(listenForKeyboardTarget.remapIndex);
		}
		
		private function arrowRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		private function arrowRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		private function onArrowClick(incr:Number, variable:Object, myArray:Array, Arrow:MovieClip)
		{
			var	p:MovieClip = (Arrow.parent as MovieClip);
			if(p.State == "disabled")
				return;
				
			var myVar:Number = variable.myValue;
			var prevMode:Number = myVar;
			myVar += incr;
			if(myVar<0)
				myVar = myArray.length-1;
			if(myVar>=myArray.length)
				myVar = 0;
	
			p.Value.Value1 = myArray[prevMode];
			p.Value.Value2 = myArray[myVar];
			p.Value.Text.text = myArray[myVar];
			p.Value.gotoAndPlay ("start");
			variable.myValue = myVar;
		}
		
		private function arrowPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				evt.currentTarget.parent.ActionFunction ();
			}
		}

		private function barPressFn(evt:MouseEvent)
		{
			var barW:Number = evt.currentTarget.width;
			var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
			var xPos = (coord.x / barW) * 10;

			setSliderValue2 (evt.currentTarget.parent, xPos);
			isMouseDown = true;
		}
		
		private function barMoveFn(evt:MouseEvent)
		{
			if (isMouseDown)
			{
				var barW:Number = evt.currentTarget.width;
				var coord:Point = evt.currentTarget.globalToLocal(new Point(evt.stageX, evt.stageY));
				var xPos = (coord.x / barW) * 10;
	
				setSliderValue2 (evt.currentTarget.parent, xPos);
			}
		}
		
		private function barUpFn(evt:MouseEvent)
		{
			isMouseDown = false;
		}
		
		private function setSliderValue2(control:MovieClip, param:Number)
		{
			var barW:Number = 550;
			
			param = Math.min (param, 10);
			param = Math.max (param, 0);	

			var	width:Number = param / 10 * barW;
			control.Bar.Scale.width = width;
			
			var	str:String = (param).toString(10);
			if(str.length > 4)
				str = str.slice(0, 4);

			control.Value.Text.text = str;
		}
		
		private function getSliderValue2(control:MovieClip):Number
		{
			var ret:Number = control.Bar.Scale.width / control.Bar.width;
			return ret*10;
		}
		
		public function handleScrollerUpDown(delta:Number)
		{
			if(delta < 0)
			{ 
				var startY = Scroller.Field.y; 
				if(Scroller.Trigger.y > startY) 
				{ 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y < startY) 
					{
						Scroller.Trigger.y = startY; 
					}
				} 
			}
			else
			{
				var endY = Scroller.Field.height - Scroller.Trigger.height + 26;
				if(Scroller.Trigger.y < endY) 
				{ 
					Scroller.Trigger.y += delta; 
					if(Scroller.Trigger.y > endY) 
					{
						Scroller.Trigger.y = endY; 
					}
				} 
			}
		}
		
		public	function scrollItems(e:Event)
		{
			var	a = Scroller.Field.y;
			var	b = Scroller.Trigger.y;
			var	dist = (Scroller.Field.y - Scroller.Trigger.y);
			var	h = keyControls.height;
			var	h1 = Scroller.Field.height - Scroller.Trigger.height;
			var	mh = OptionsControlsClip.Mask.height - 5;
			h -= mh;
			h /= h1;
			
			var	dest:Number = dist * h;
			var	step:Number = int (dest / 51);
			
			if (keyControls.y != 51 * step)
			{
				Tweener.addTween(keyControls, {y:51 * step, time:api.tweenDelay, transition:"linear"});
			}
		}
	}
}
