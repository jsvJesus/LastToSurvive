package warz.events  
{
	import warz.events.Events;
	import flash.display.MovieClip;
	
	public class LoadingScreenEvents extends Events
	{
		public static function eventRegisterUI(main:MovieClip)
		{
			send("eventRegisterUI", main);
		}
	}
}