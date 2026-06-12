package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the HUD that may or may not be specific to a single screen
	 */
	
	public class HUDEvents extends Events
	{
		public static function eventGraveNoteClosed()
		{
			send("eventGraveNoteClosed");
		}
		public static function eventSafelockPass(code:String)
		{
			send("eventSafelockPass", code);
		}
		public static function eventShowPlayerListContextMenu(gamertag:String)
		{
			send("eventShowPlayerListContextMenu", gamertag);
		}
		
		public static function eventPlayerListAction(actionID:int, gamertag:String)
		{
			send("eventPlayerListAction", actionID, gamertag);
		}
		public static function eventChatMessage(tabID:int, msg:String)
		{
			send("eventChatMessage", tabID, msg);
		}
		
		public static function eventNoteWritePost(msg:String)
		{
			send("eventNoteWritePost", msg);
		}
		
		public static function eventNoteReportAbuse()
		{
			send("eventNoteReportAbuse");
		}
		
		public static function eventNoteClosed()
		{
			send("eventNoteClosed");
		}
	}
}