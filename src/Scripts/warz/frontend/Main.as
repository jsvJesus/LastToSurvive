package warz.frontend {
	import flash.display.MovieClip;
	
	public class Main extends MovieClip {
		public var MainMenu:MovieClip;
		
		public var SurvivorsAnim:MovieClip;
		public var Marketplace:MovieClip;
		public var Inventory:MovieClip;
		public var CreateSurv:MovieClip;
		public var OptionsLang:MovieClip;
		public var OptionsVoip:MovieClip;
		public var OptionsControl:MovieClip;
		public var OptionsGeneral:MovieClip;
		public var LogIn:MovieClip;
		public var MsgBox:MovieClip;
		public var Community:MovieClip;
		public var CreateSurvPopup:MovieClip;
		public var PopupChangebp:MovieClip;
		public var PlayGame:MovieClip;
		public var PlayGameQuickJoin:MovieClip;
		public var BrowseGamesAnim:MovieClip;
		public var Clans:MovieClip;
		public var ClansCreateClan:MovieClip;
		public var ClansMyClan:MovieClip;
		public var ClansMyClanApps:MovieClip;
		public var ClanInvitePopup:MovieClip;
		public var PurchaseGC:MovieClip;
		public var PopUpEarlyRevival:MovieClip;
		public var LeaderboardAnim:MovieClip;
		public var PlayGameMyServers:MovieClip;
		public var RentServerPopup:MovieClip;
		public var SkillTree:MovieClip;
		public var ChangeOutfit:MovieClip;
		public var BrowseGamesChannelsAnim:MovieClip;
		public var PremiumNeededPopUp:MovieClip;
		public var DonateGCSrvPopUp:MovieClip;

		public var ActiveScreen			:MovieClip = null;
		public var activeScreenName		:String = "empty";
		public var PrevActiveScreen 	:MovieClip = null;

		public function Main() {
			MainMenu.visible = true;
			LogIn.visible = false;
			CreateSurvPopup.visible = false;
			MsgBox.visible = false;
			Community.visible = false;
			CreateSurvPopup.visible = false;
			PopupChangebp.visible = false;
			PlayGame.visible = false;
		}

		public function showScreen(screen:String)
		{
			//trace("showScreen="+screen);
			if(screen == activeScreenName)
				return;
			var prevName = activeScreenName;
			activeScreenName = screen;
			
			//trace("showScreen="+screen);
			//trace("prevName="+prevName);
			
			switch (screen)
			{
				case	"Survivors":
					switchScreen(SurvivorsAnim);
					break;
					
				case	"MarketPlace":
					switchScreen(Marketplace);
					break;
					
				case	"Inventory":
					switchScreen(Inventory);
					break;
					
				case	"CreateSurvivor":
					switchScreen(CreateSurv);
					break;
					
				case	"ChangeOutfit":
					switchScreen(ChangeOutfit);
					break;
					
				case	"OptionsLang":
					switchScreen(OptionsLang);
					break;

				case	"OptionsVoip":
					switchScreen(OptionsVoip);
					break;

				case	"OptionsGeneral":
					switchScreen(OptionsGeneral);
					break;
					
				case	"OptionsControl":
					switchScreen(OptionsControl);
					break;
					
				case	"Community":
					//switchScreen(Community);
					//switchScreen(Clans);
					switchScreen(LeaderboardAnim);
					break;
					
				case 	"PlayGame":
					switchScreen(BrowseGamesChannelsAnim);
					break;
					
				case 	"PlayGameQuick":
					switchScreen(PlayGameQuickJoin);
					break;
					
				case	"ServerBrowse":
					switchScreen(BrowseGamesAnim);
					break;
					
				case	"MyServers":
					switchScreen(PlayGameMyServers);
					break;
					
				case 	"RentGameServer":
					RentServerPopup.isGameServerRenting = 1;
					switchScreen(RentServerPopup);
					break;
				case 	"RentStrongholdServer":
					RentServerPopup.isGameServerRenting = 0;
					switchScreen(RentServerPopup);
					break;
					
				case	"RentRenewServer":
					RentServerPopup.isGameServerRenting = 2;
					switchScreen(RentServerPopup);
					break;
					
				case 	"Clans":
					switchScreen(Clans);
					break;
					
				case	"CreateClan":
					switchScreen(ClansCreateClan);
					break;
					
				case	"MyClan":
					switchScreen(ClansMyClan);
					break;
					
				case	"MyClanAdmin":
					switchScreen(ClansMyClanApps);
					break;
					
				case	"Leaderboard":
					switchScreen(LeaderboardAnim);
					break;
					
				case	"SkillTree":
					switchScreen(SkillTree);
					break;
					
				default:
					trace("[ERROR] showScreen: unknown screen - "+screen);
					break;
			}
		}
	
		private function switchScreen(newScreen:MovieClip)
		{
			if(ActiveScreen != null) // fade out previous screen
			{
				ActiveScreen.Deactivate();
				ActiveScreen.isActiveScreen = false;
			}
			PrevActiveScreen = ActiveScreen;
			
			ActiveScreen = newScreen;
			ActiveScreen.isActiveScreen = true;
			ActiveScreen.Activate();
		}
		
		public function showPrevScreen()
		{
			if(PrevActiveScreen)
				switchScreen(PrevActiveScreen);
			else
				showScreen("Survivors");
		}
		
		public function updateSurvivors ()
		{
			if (ActiveScreen == SurvivorsAnim ||
				ActiveScreen == CreateSurv || 
				ActiveScreen == ChangeOutfit)
			{
				ActiveScreen.updateSurvivors ();
			}
		}
		
		public	function updateDeadTimer (seconds:int, percent:int, reviveBtnVis:Boolean)
		{
			if (ActiveScreen == SurvivorsAnim)
			{
				ActiveScreen.updateDeadTimer (seconds, percent, reviveBtnVis);
			}
		}
		
		public	function buyItemSuccessful ()
		{
			if (ActiveScreen == Marketplace ||
				ActiveScreen == CreateSurv)
			{
				ActiveScreen.buyItemSuccessful ();
			}
		}
		
		public function showInfoOkCancelMsg(msg:String, title:String = "", callback:* = null)
		{
			MsgBox.showInfoOkCancelMsg (msg, title, callback);
		}
		
		public function showInfoMsg(msg:String, OKButton:Boolean, title:String = "")
		{
			MsgBox.showInfoMsg (msg, OKButton, title);
		}
		
		public function showCharacterUnlock (charID:int):void
		{
			CreateSurvPopup.showCharacterUnlock (charID);
		}
		
		public	function showChangeBackpack ():void
		{
			PopupChangebp.Activate ();
		}
	}	
}
