package warz.utils
{
	import flash.events.Event;
	import flash.net.URLRequest;
	import flash.display.Loader;
	import flash.display.Bitmap;

	public class ImageLoader
	{
		public	var	imageData:Object;
		public	var	callback:*;
		
		public function ImageLoader(path, callBack:*, iData:Object)
		{
			imageData = iData;
			callback = callBack;
			
			var req:URLRequest = new URLRequest(path);
			var loader:Loader = new Loader();  
			loader.load( req );  

			loader.contentLoaderInfo.addEventListener( Event.COMPLETE, onComplete );
		}
					
		public	function onComplete (e:Event)
		{
			var	bitmap:Bitmap = e.target.content as Bitmap;
			bitmap.smoothing=true;
			if (callback)
				callback (bitmap, imageData);
		}
	}
}
