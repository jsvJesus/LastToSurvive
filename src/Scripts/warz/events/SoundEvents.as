package warz.events  
{
	import warz.events.Events;
	
	/**
	 * Events dispatched by the FrontEnd that may or may not be specific to a single screen
	 */
	
	public class SoundEvents extends Events
	{
		public static function eventSoundPlay(sound:String)
		{
			send("eventSoundPlay", sound);
		}
	}
}