package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the Pause that may or may not be specific to a single screen
	 */
	
	public class TradeEvents extends Events
	{
		public static function eventReturnToGame()
		{
			send("eventReturnToGame");
		}
		
		public static function eventBackpackToTrade(gridFrom:int, TradeGridTo:int, amount:int)
		{
			send("eventBackpackToTrade", gridFrom, TradeGridTo, amount);
		}
		public static function eventTradeToBackpack(TradeGridFrom:int, gridTo:int, amount:int)
		{
			send("eventTradeToBackpack", TradeGridFrom, gridTo, amount);
		}		
		public static function eventBackpackGridSwap (gridFrom:int, gridTo:int)
		{
			send("eventBackpackGridSwap", gridFrom, gridTo);
		}		
		public static function eventTradeGridSwap (gridFrom:int, gridTo:int)
		{
			send("eventTradeGridSwap", gridFrom, gridTo);
		}
		public static function eventTradeAccept()
		{
			send("eventTradeAccept");
		}
		public static function eventTradeDecline()
		{
			send("eventTradeDecline");
		}
	}
}