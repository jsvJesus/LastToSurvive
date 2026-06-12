package warz.attachments {
	import flash.display.MovieClip;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import warz.events.AttachmentEvents;
	
	public class attachmentIcon extends MovieClip {
		
		public	var		Circle:MovieClip;
		
		public	var		slotID:int
		
		public function attachmentIcon() {
			init ();
		}

		public function init(id:int = 0) {		
			slotID = id;
			
			for (var a = 1; a <= 10; a++)
			{
				var	name:String = "Slot" + String (a);
				this[name].visible = false;
			}
			
			//Circle.Text.Text.text = slotID;
			Circle.Text.gotoAndStop(id+1);
			Circle.gotoAndPlay(0);
		}
		
		public	function addSlot (slotID:int, slotName:String, attachmentID:int):void
		{
			if (slotID >= 10)
				return;
				
			var	name:String = "Slot" + String (slotID + 1);
			
			this[name].Btn.addEventListener(MouseEvent.MOUSE_OVER, rollOverFn);
			this[name].Btn.addEventListener(MouseEvent.MOUSE_OUT, rollOutFn);
			this[name].Btn.addEventListener(MouseEvent.CLICK, pressFn);
			this[name].attachmentID = attachmentID;
			this[name].slotID = slotID;
			
			this[name].Text.Text.text = slotName;
			this[name].visible = true;
		}
		
		public	function setSlotActive (slotID:int)
		{
			if (slotID >= 10)
				return;

			for (var a = 1; a <= 10; a++)
			{
				var	name:String = "Slot" + String (a);
				
				if (this[name].State == "active")
				{
					if (this[name].currentLabel != "off")
						this[name].gotoAndPlay("off");
					this[name].State = "off";
				}
			}

			name = "Slot" + String (slotID + 1);
			
			if (this[name].currentLabel != "hilite")
				this[name].gotoAndPlay ("hilite");
				
			this[name].State = "active";
		}
		
		private function rollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("on");
		}
		
		private function rollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("off");
		}
		
		private function pressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				for (var a = 1; a <= 10; a++)
				{
					var	name:String = "Slot" + String (a);
					
					if (this[name].State == "active")
					{
						this[name].gotoAndPlay("off");
						this[name].State = "off";
					}
				}
				
				evt.currentTarget.parent.State = "active";
				evt.currentTarget.parent.gotoAndPlay("hilite");
				
				AttachmentEvents.eventSelectAttachment (slotID, evt.currentTarget.parent.slotID, evt.currentTarget.parent.attachmentID);
			}
		}		
		
		public	function removeEvents ():void
		{
			for (var a = 1; a <= 10; a++)
			{
				var	name="Slot" + String (a);
				
				this[name].Btn.removeEventListener(MouseEvent.MOUSE_OVER, rollOverFn);
				this[name].Btn.removeEventListener(MouseEvent.MOUSE_OUT, rollOutFn);
				this[name].Btn.removeEventListener(MouseEvent.CLICK, pressFn);
			}
		}
	}
}
