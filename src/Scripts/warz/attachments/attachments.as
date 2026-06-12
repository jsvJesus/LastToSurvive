package warz.attachments {
	import flash.display.MovieClip;
	
	public class attachments extends MovieClip {
		public var isDebug:Boolean = false;
		
		public var Main:MovieClip = null;

		static public var api:attachments = null;
		
		public function attachments(main:MovieClip) 
		{
			api = this;
			Main = main;
			Main.init ();
		}
		
		public	function clearAttachments ():void
		{
			Main.clearAttachments ();
		}
		
		public	function addAttachment (id:int, x:int, y:int, num:int):void
		{
			Main.addAttachment (id, x, y, num);
		}
		
		public	function addSlot (id:int, slotID:int, name:String, attachmentID:int):void
		{
			Main.addSlot (id, slotID, name, attachmentID);
		}
		
		public	function setSlotActive (id:int, slotID:int)
		{
			Main.setSlotActive (id, slotID);
		}
	}
}