package warz.frontend {
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import warz.events.FrontEndEvents;
	import warz.events.SoundEvents;
	import warz.dataObjects.StoreItem;
	import warz.dataObjects.Item;
	import warz.dataObjects.Hero;
	import warz.utils.ImageLoader;
	import flash.display.Bitmap;
	import warz.utils.Layout;
	import flash.events.Event;
	
	public class charPopup extends MovieClip {
		
		public	var		CreateSurvPopup:MovieClip;
		
		public 	var 	api:warz.frontend.Frontend=null;
		public	var		charID:int;
		public	var		charStoreItem:StoreItem;
		public	var		charItem:Hero;
		
		public	function charPopup ()
		{
		}
		
		public	function showCharacterUnlock(charID:int):void
		{
			api = warz.frontend.Frontend.api;
			
			visible = true;
			
			this.charID = charID;
			charStoreItem = api.getStoreItemByID (charID);
			for(var i=0; i<api.HeroDB.length; ++i)
				if(api.HeroDB[i].itemID == charID)
				{
					charItem = api.HeroDB[i];
					break;
				}

			refreshCharacterUnlock ();
		}
		
		public	function refreshCharacterUnlock ():void
		{			
		
			CreateSurvPopup.BtnGC.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			CreateSurvPopup.BtnGC.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			CreateSurvPopup.BtnGC.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			CreateSurvPopup.BtnGC.Text.Text.text = "$FR_CREATE_CHARACTER_PURCHASE_GC";
			
			CreateSurvPopup.X.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
			CreateSurvPopup.X.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
			CreateSurvPopup.X.Btn.addEventListener(MouseEvent.CLICK, ButtonPressFn);
			
			if (CreateSurvPopup.BlockGC.currentLabel != "gc")
				CreateSurvPopup.BlockGC.gotoAndPlay ("gc");
			CreateSurvPopup.BlockGC.BtnUnlock.Text.Text.text = "$FR_Unlock";
			CreateSurvPopup.BlockGC.TitleCurrent.text = "$FR_CREATE_CHARACTER_CURRENT_GC";
			CreateSurvPopup.BlockGC.TitleCost.text = "$FR_CREATE_CHARACTER_COST";
			CreateSurvPopup.BlockGC.TitleBalance.text = "$FR_CREATE_CHARACTER_BALANCE";
			CreateSurvPopup.BlockGC.Current.text = String (api.money.gc);
			CreateSurvPopup.BlockGC.Cost.text = String (charStoreItem.price);
			CreateSurvPopup.BlockGC.Balance.text = String (api.money.gc - charStoreItem.price);
			CreateSurvPopup.BlockGC.Title.text = "$FR_CREATE_CHARACTER_TITLE_GC";
			if (api.money.gc - charStoreItem.price >= 0)
			{
				CreateSurvPopup.BlockGC.BtnUnlock.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
				CreateSurvPopup.BlockGC.BtnUnlock.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
				CreateSurvPopup.BlockGC.BtnUnlock.Btn.addEventListener(MouseEvent.CLICK, BlockGCBtnUnlock);
			}

			if (CreateSurvPopup.BlockXP.currentLabel != "gd")
				CreateSurvPopup.BlockXP.gotoAndPlay ("gd");
			CreateSurvPopup.BlockXP.BtnUnlock.Text.Text.text = "$FR_Unlock";
			CreateSurvPopup.BlockXP.TitleCurrent.text = "$FR_CREATE_CHARACTER_CURRENT_XP";
			CreateSurvPopup.BlockXP.TitleCost.text = "$FR_CREATE_CHARACTER_COST";
			CreateSurvPopup.BlockXP.TitleBalance.text = "$FR_CREATE_CHARACTER_BALANCE";
			CreateSurvPopup.BlockXP.Current.text = String (api.money.dollars);
			CreateSurvPopup.BlockXP.Cost.text = String (charStoreItem.priceGD);
			CreateSurvPopup.BlockXP.Balance.text = String (api.money.dollars - charStoreItem.priceGD);
			CreateSurvPopup.BlockXP.Title.text = "$FR_CREATE_CHARACTER_TITLE_XP";
			if (api.money.dollars - charStoreItem.priceGD >= 0)
			{
				CreateSurvPopup.BlockXP.BtnUnlock.Btn.addEventListener(MouseEvent.MOUSE_OVER, ButtonRollOverFn);
				CreateSurvPopup.BlockXP.BtnUnlock.Btn.addEventListener(MouseEvent.MOUSE_OUT, ButtonRollOutFn);
				CreateSurvPopup.BlockXP.BtnUnlock.Btn.addEventListener(MouseEvent.CLICK, BlockXPBtnUnlock);
			}
			
			CreateSurvPopup.Title.text = "$FR_CREATE_CHARACTER_UNLOCK_CHARACTER";
			CreateSurvPopup.Name.text = charItem.name;
			
			if (CreateSurvPopup.Character.numChildren > 0)
	 			CreateSurvPopup.Character.removeChildAt (0);
			loadSlotIcon (charItem.icon3, CreateSurvPopup.Character);
		}
		
		public	function buyCharacter (xp:Boolean):void
		{
			if (api.isDebug)
			{
				api.addInventoryItem(api.InventoryDB.length, charID, 1, 0, 0, false);
				api.money.gc -= charStoreItem.price;
				api.money.dollars -= charStoreItem.priceGD;
					
				api.buyItemSuccessful ();
			}
			else 
			{
				if (xp)
					FrontEndEvents.eventBuyItem (charID, 0, charStoreItem.priceGD);
				else
					FrontEndEvents.eventBuyItem (charID, charStoreItem.price, 0);
			}
		}
		
		public function loadSlotIcon(path:String, imageHolder:MovieClip)
		{
			var	dat:Object = {imageHolder:imageHolder};
			var	imageLoader:ImageLoader = new ImageLoader (path, loadSlotCallback, dat)
		}
		
		public	function loadSlotCallback (bitmap:Bitmap, dat:Object)
		{
			var slotWidth=0, slotHeight=0;
			var slotX=0, slotY=0;
			
			slotWidth = 512;
			slotHeight = 1024;
			slotX = 0;
			slotY = 0;
			
			bitmap = Layout.stretch (bitmap, slotWidth, slotHeight, bitmap.width, bitmap.height, "uniform");
			
			bitmap.x = slotX; 
			bitmap.y = slotY; 

			dat.imageHolder.addChild (bitmap);
		}
		
		public	function ButtonRollOverFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("over");
		}
		
		public	function ButtonRollOutFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
				evt.currentTarget.parent.gotoAndPlay("out");
		}
		
		public	function ButtonPressFn(evt:Event)
		{
			if(evt.currentTarget.parent.State != "active")
			{
				var	p:MovieClip = evt.currentTarget.parent;
				
				while (!(p as charPopup))
				{
					p = p.parent as MovieClip;
				}
				
				SoundEvents.eventSoundPlay("menu_click");
				
				(p as charPopup).ActionFunction(evt.currentTarget.parent.name);
			}
		}
		
		public	function ActionFunction (button:String):void
		{
			if (button == "BtnGC")
			{
				FrontEndEvents.eventStorePurchaseGPRequest();
				//api.Main.PurchaseGC.showPopUp();
			}
			else if (button == "BlockXP")
			{
				buyCharacter (true);
			}
			else if (button == "BlockGC")
			{
				buyCharacter (false);
			}
			else if (button == "X")
			{
				visible = false;
			}
		}
		
		public	function BlockGCBtnUnlock (e:Event):void
		{
			SoundEvents.eventSoundPlay("menu_click");
			buyCharacter (false);
			visible = false;
		}

		public	function BlockXPBtnUnlock (e:Event):void
		{
			SoundEvents.eventSoundPlay("menu_click");
			buyCharacter (true);
			visible = false;
		}
	}
}