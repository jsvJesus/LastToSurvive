package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the Pause that may or may not be specific to a single screen
	 */
	
	public class PauseEvents extends Events
	{
		public static function eventMissionRequestList()
		{
			send("eventMissionRequestList");
		}
		public static function eventMissionAccept(missionID:uint)
		{
			send("eventMissionAccept", missionID);
		}
		public static function eventMissionDecline(missionID:uint)
		{
			send("eventMissionDecline", missionID);
		}
		public static function eventMissionAbandon(missionID:uint)
		{
			send("eventMissionAbandon", missionID);
		}
		
		public static function eventRepairItem(slotID:int, kit:int)
		{
			send("eventRepairItem", slotID, kit);
		}
		
		public static function eventShowContextMenuCallback(itemID:uint, slotID:int)
		{
			send("eventShowContextMenuCallback", itemID, slotID);
		}
		public static function eventContextMenu_Action(slotID:int, ActionID:int)
		{
			send("eventContextMenu_Action", slotID, ActionID);
		}
		
		public static function eventDisableHotKeys(st:Boolean)
		{
			send("eventDisableHotKeys", st);
		}
		public static function eventSendCallForHelp(distress:String, reward:String)
		{
			send("eventSendCallForHelp", distress, reward);
		}
		public static function eventBackpackGridSwap (gridFrom:int, gridTo:int)
		{
			send("eventBackpackGridSwap", gridFrom, gridTo);
		}		
		/*public static function eventBackpackDrop (slotID:int)
		{
			send("eventBackpackDrop", slotID);
		}
		public static function eventBackpackUnloadClip (slotID:int)
		{
			send("eventBackpackUnloadClip", slotID);
		}
		public static function eventBackpackUseItem (slotID:int)
		{
			send("eventBackpackUseItem", slotID);
		}		*/
		public static function eventMsgBoxCallback ()
		{
			send ("eventMsgBoxCallback");
		}
		public static function eventBackToGame ()
		{
			send ("eventBackToGame");
		}
		public static function eventQuitGame ()
		{
			send ("eventQuitGame");
		}
		public static function eventShowMap()
		{
			send ("eventShowMap");
		}
		public static function eventShowOptions ()
		{
			send ("eventShowOptions");
		}
		public static function eventChangeBackpack (slotID:int, itemID:uint)
		{
			send ("eventChangeBackpack", slotID, itemID);
		}
		public static function eventOptionsControlsRequestKeyRemap(index:Number)
		{
			send("eventOptionsControlsRequestKeyRemap", index);
		}
		public static function eventOptionsControlsReset():void
		{
			send("eventOptionsControlsReset");
		}
		public static function eventOptionsApply (mouseSens:Number, brightness:Number, contrast:Number, 
											  soundV:Number, musicV:Number, commV:Number,
											  overallQ:Number, hintS:Number, vertLook:Number,
											  mouseWheel:Number, mouseAccel:Number, RedBlood:Number):void
		{
			send("eventOptionsApply", mouseSens, brightness, contrast, soundV, musicV, commV,
										overallQ, hintS, vertLook, mouseWheel, mouseAccel, RedBlood);
		}
	}
}