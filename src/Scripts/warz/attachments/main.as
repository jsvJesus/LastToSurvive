package warz.attachments {
	import flash.display.MovieClip;
	import warz.events.SoundEvents;
	
	public class main extends MovieClip {
		
		public	var api:warz.attachments.attachments = null;
		
		public	var	Attachments:Array;

		public function main() {
			test.visible = false;
		}
		
		public	function init ():void
		{
			api = warz.attachments.attachments.api;
			Attachments = new Array ();
		}
		
		public	function clearAttachments ():void
		{
			for (var a:int = 0; a < Attachments.length; a++)
			{
				removeAttachment (a);
			}
		}

		public	function removeAttachment (id:int):void
		{
			var	attach:attachmentIcon = Attachments[id];
			
			if (attach)
			{
				removeChild (attach);
				
				attach.removeEvents ();
				Attachments[id] = null;
			}
		}
		
		public	function addAttachment (id:int, x:int, y:int, num:int):void
		{
			var	attach:attachmentIcon = new attachmentIcon ();
			attach.init (num);
			attach.x = x;
			attach.y = y;
			
			Attachments[id] = attach;
			addChild (attach);
		}
		
		public	function addSlot (id:int, slotID:int, name:String, attachmentID:int):void
		{
			var	attach:attachmentIcon = Attachments[id];
			
			if (attach)
			{
				attach.addSlot (slotID, name, attachmentID);
			}
		}
		
		public	function setSlotActive (id:int, slotID:int)
		{
			var	attach:attachmentIcon = Attachments[id];
			
			if (attach)
			{
				attach.setSlotActive (slotID);
				
				SoundEvents.eventSoundPlay("weapon_attach");
			}
		}
	}
}
