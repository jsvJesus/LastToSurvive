package warz.frontend {
	import flash.display.MovieClip;
	
	public class community extends MovieClip {
		public var isActiveScreen:Boolean = false;

		public	function community ()
		{
			visible = false;
		}
		
		public function Activate()
		{
			this.visible = true;
		}
		
		public function Deactivate()
		{
			this.visible = false;
		}
	}
}