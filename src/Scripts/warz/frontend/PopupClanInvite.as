package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import flash.geom.Rectangle;
	import flash.display.Bitmap;
	import warz.utils.ImageLoader;
	import warz.utils.Layout;
	
	public class PopupClanInvite extends MovieClip {
		
		private var clanInviteID:uint = 0;
		public	function PopupClanInvite ()
		{
			visible = false;
		}
		
		public function showInvite(inviteID:uint, clanName:String, numMembers:uint, desc:String, icon:String)
		{
			clanInviteID = inviteID;
			this.ClanName.text = clanName;
			this.MemberNum.text = "$FR_CLANMEMBERS2"+numMembers;
			this.Descr.text = desc;
			if(this.ClanIcon.numChildren > 0)
				this.ClanIcon.removeChildAt(0);
			loadSlotIcon(icon, this.ClanIcon, 0);
			
			visible = true;
			
			this.BtnDecline.Text.Text.text = "$FR_DECLINE";
			this.BtnAccept.Text.Text.text = "$FR_ACCEPT";
			
			this.BtnAccept.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			this.BtnAccept.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			this.BtnAccept.Btn.addEventListener(MouseEvent.CLICK, MouseOkClick);

			this.BtnDecline.Btn.addEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			this.BtnDecline.Btn.addEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			this.BtnDecline.Btn.addEventListener(MouseEvent.CLICK, MouseCancelClick);
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

		public	function MouseOkClick (evt:MouseEvent):void
		{			
			SoundEvents.eventSoundPlay("menu_click");
			
			this.BtnAccept.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			this.BtnAccept.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			this.BtnAccept.Btn.removeEventListener(MouseEvent.CLICK, MouseOkClick);
			
			this.BtnDecline.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			this.BtnDecline.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			this.BtnDecline.Btn.removeEventListener(MouseEvent.CLICK, MouseCancelClick);

			visible = false; 
			FrontEndEvents.eventClanRespondToInvite(clanInviteID, true);
		}

		public	function MouseCancelClick (evt:MouseEvent):void
		{
			SoundEvents.eventSoundPlay("menu_click");
			
			this.BtnAccept.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			this.BtnAccept.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			this.BtnAccept.Btn.removeEventListener(MouseEvent.CLICK, MouseOkClick);
			
			this.BtnDecline.Btn.removeEventListener(MouseEvent.MOUSE_OVER, MouseOver);
			this.BtnDecline.Btn.removeEventListener(MouseEvent.MOUSE_OUT, MouseOut);
			this.BtnDecline.Btn.removeEventListener(MouseEvent.CLICK, MouseCancelClick);

			visible = false; 
			FrontEndEvents.eventClanRespondToInvite(clanInviteID, false);
		}
	}
}