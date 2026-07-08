/*
 Navicat Premium Dump SQL

 Source Server         : WarZ
 Source Server Type    : SQL Server
 Source Server Version : 11003156 (11.00.3156)
 Source Host           : localhost:1433
 Source Catalog        : LTS
 Source Schema         : dbo

 Target Server Type    : SQL Server
 Target Server Version : 11003156 (11.00.3156)
 File Encoding         : 65001

 Date: 02/07/2026 23:50:27
*/


-- ----------------------------
-- Table structure for Accounts
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Accounts]') AND type IN ('U'))
	DROP TABLE [dbo].[Accounts]
GO

CREATE TABLE [dbo].[Accounts] (
  [CustomerID] int  IDENTITY(1000000,1) NOT NULL,
  [email] varchar(128) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [MD5Password] varchar(32) COLLATE Cyrillic_General_CI_AS  NULL,
  [dateregistered] datetime  NULL,
  [ReferralID] int  NOT NULL,
  [AccountStatus] int DEFAULT 100 NOT NULL,
  [IsDeveloper] int DEFAULT 0 NOT NULL,
  [lastlogindate] datetime  NULL,
  [lastloginIP] varchar(16) COLLATE Cyrillic_General_CI_AS  NULL
)
GO

ALTER TABLE [dbo].[Accounts] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Accounts
-- ----------------------------
SET IDENTITY_INSERT [dbo].[Accounts] ON
GO

INSERT INTO [dbo].[Accounts] ([CustomerID], [email], [MD5Password], [dateregistered], [ReferralID], [AccountStatus], [IsDeveloper], [lastlogindate], [lastloginIP]) VALUES (N'1000000', N'test@gmail.com', N'f58e5dca65a955d170ab0d5f34ed32a1', N'2026-06-15 00:01:07.320', N'0', N'100', N'126', N'2026-07-02 23:40:11.697', N'26.163.92.76')
GO

SET IDENTITY_INSERT [dbo].[Accounts] OFF
GO


-- ----------------------------
-- Table structure for Achievements
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Achievements]') AND type IN ('U'))
	DROP TABLE [dbo].[Achievements]
GO

CREATE TABLE [dbo].[Achievements] (
  [CustomerID] int  NOT NULL,
  [AchID] int  NOT NULL,
  [Value] int DEFAULT 0 NOT NULL,
  [Unlocked] int DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[Achievements] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Achievements
-- ----------------------------

-- ----------------------------
-- Table structure for CharsStats
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[CharsStats]') AND type IN ('U'))
	DROP TABLE [dbo].[CharsStats]
GO

CREATE TABLE [dbo].[CharsStats] (
  [CharID] int  NOT NULL,
  [PlayersKilled] int DEFAULT 0 NOT NULL,
  [ZombiesKilled] int DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[CharsStats] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of CharsStats
-- ----------------------------

-- ----------------------------
-- Table structure for CheatLog
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[CheatLog]') AND type IN ('U'))
	DROP TABLE [dbo].[CheatLog]
GO

CREATE TABLE [dbo].[CheatLog] (
  [ID] int  IDENTITY(1,1) NOT NULL,
  [SessionID] bigint  NOT NULL,
  [CustomerID] int  NOT NULL,
  [CheatID] int  NOT NULL,
  [ReportTime] datetime  NOT NULL
)
GO

ALTER TABLE [dbo].[CheatLog] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of CheatLog
-- ----------------------------
SET IDENTITY_INSERT [dbo].[CheatLog] ON
GO

SET IDENTITY_INSERT [dbo].[CheatLog] OFF
GO


-- ----------------------------
-- Table structure for ClanApplications
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[ClanApplications]') AND type IN ('U'))
	DROP TABLE [dbo].[ClanApplications]
GO

CREATE TABLE [dbo].[ClanApplications] (
  [ClanApplicationID] int  IDENTITY(1,1) NOT NULL,
  [ClanID] int  NOT NULL,
  [CharID] int  NOT NULL,
  [ExpireTime] datetime  NOT NULL,
  [ApplicationText] nvarchar(500) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [IsProcessed] int  NOT NULL
)
GO

ALTER TABLE [dbo].[ClanApplications] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of ClanApplications
-- ----------------------------
SET IDENTITY_INSERT [dbo].[ClanApplications] ON
GO

SET IDENTITY_INSERT [dbo].[ClanApplications] OFF
GO


-- ----------------------------
-- Table structure for ClanData
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[ClanData]') AND type IN ('U'))
	DROP TABLE [dbo].[ClanData]
GO

CREATE TABLE [dbo].[ClanData] (
  [ClanID] int  IDENTITY(1472,1) NOT NULL,
  [ClanName] nvarchar(64) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [ClanNameColor] int  NOT NULL,
  [ClanTag] nvarchar(4) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [ClanTagColor] int  NOT NULL,
  [ClanEmblemID] int  NOT NULL,
  [ClanEmblemColor] int  NOT NULL,
  [ClanXP] int  NOT NULL,
  [ClanLevel] int  NOT NULL,
  [ClanGP] int  NOT NULL,
  [OwnerCustomerID] int  NOT NULL,
  [OwnerCharID] int  NOT NULL,
  [MaxClanMembers] int  NOT NULL,
  [NumClanMembers] int  NOT NULL,
  [ClanLore] nvarchar(512) COLLATE Cyrillic_General_CI_AS  NULL,
  [Announcement] nvarchar(512) COLLATE Cyrillic_General_CI_AS  NULL,
  [ClanCreateDate] datetime  NULL
)
GO

ALTER TABLE [dbo].[ClanData] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of ClanData
-- ----------------------------
SET IDENTITY_INSERT [dbo].[ClanData] ON
GO

SET IDENTITY_INSERT [dbo].[ClanData] OFF
GO


-- ----------------------------
-- Table structure for ClanEvents
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[ClanEvents]') AND type IN ('U'))
	DROP TABLE [dbo].[ClanEvents]
GO

CREATE TABLE [dbo].[ClanEvents] (
  [ClanEventID] int  IDENTITY(1,1) NOT NULL,
  [ClanID] int  NOT NULL,
  [EventDate] datetime  NOT NULL,
  [EventType] int  NOT NULL,
  [EventRank] int  NOT NULL,
  [Var1] int DEFAULT 0 NOT NULL,
  [Var2] int DEFAULT 0 NOT NULL,
  [Var3] int DEFAULT 0 NOT NULL,
  [Var4] int DEFAULT 0 NOT NULL,
  [Text1] nvarchar(64) COLLATE Cyrillic_General_CI_AS  NULL,
  [Text2] nvarchar(64) COLLATE Cyrillic_General_CI_AS  NULL,
  [Text3] nvarchar(256) COLLATE Cyrillic_General_CI_AS  NULL
)
GO

ALTER TABLE [dbo].[ClanEvents] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of ClanEvents
-- ----------------------------
SET IDENTITY_INSERT [dbo].[ClanEvents] ON
GO

SET IDENTITY_INSERT [dbo].[ClanEvents] OFF
GO


-- ----------------------------
-- Table structure for ClanInvites
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[ClanInvites]') AND type IN ('U'))
	DROP TABLE [dbo].[ClanInvites]
GO

CREATE TABLE [dbo].[ClanInvites] (
  [ClanInviteID] int  IDENTITY(1,1) NOT NULL,
  [ClanID] int  NOT NULL,
  [InviterCharID] int  NOT NULL,
  [CharID] int  NOT NULL,
  [ExpireTime] datetime  NOT NULL
)
GO

ALTER TABLE [dbo].[ClanInvites] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of ClanInvites
-- ----------------------------
SET IDENTITY_INSERT [dbo].[ClanInvites] ON
GO

SET IDENTITY_INSERT [dbo].[ClanInvites] OFF
GO


-- ----------------------------
-- Table structure for DataGameRewards
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DataGameRewards]') AND type IN ('U'))
	DROP TABLE [dbo].[DataGameRewards]
GO

CREATE TABLE [dbo].[DataGameRewards] (
  [ID] int  NOT NULL,
  [Name] nvarchar(128) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [GD_SOFT] int DEFAULT 0 NOT NULL,
  [XP_SOFT] int DEFAULT 0 NOT NULL,
  [GD_HARD] int DEFAULT 0 NOT NULL,
  [XP_HARD] int DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[DataGameRewards] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DataGameRewards
-- ----------------------------

-- ----------------------------
-- Table structure for DataSkill2Price
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DataSkill2Price]') AND type IN ('U'))
	DROP TABLE [dbo].[DataSkill2Price]
GO

CREATE TABLE [dbo].[DataSkill2Price] (
  [SkillID] int  NOT NULL,
  [SkillName] varchar(64) COLLATE Cyrillic_General_CI_AS DEFAULT 'SKILL DESC' NULL,
  [Lv1] int DEFAULT 0 NULL,
  [Lv2] int DEFAULT 0 NULL,
  [Lv3] int DEFAULT 0 NULL,
  [Lv4] int DEFAULT 0 NULL,
  [Lv5] int DEFAULT 0 NULL
)
GO

ALTER TABLE [dbo].[DataSkill2Price] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DataSkill2Price
-- ----------------------------

-- ----------------------------
-- Table structure for DBG_BanLog
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_BanLog]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_BanLog]
GO

CREATE TABLE [dbo].[DBG_BanLog] (
  [RecordID] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NOT NULL,
  [BanTime] datetime  NULL,
  [BanDuration] int  NULL,
  [BanReason] nvarchar(512) COLLATE Cyrillic_General_CI_AS  NULL
)
GO

ALTER TABLE [dbo].[DBG_BanLog] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_BanLog
-- ----------------------------
SET IDENTITY_INSERT [dbo].[DBG_BanLog] ON
GO

SET IDENTITY_INSERT [dbo].[DBG_BanLog] OFF
GO


-- ----------------------------
-- Table structure for DBG_GPTransactions
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_GPTransactions]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_GPTransactions]
GO

CREATE TABLE [dbo].[DBG_GPTransactions] (
  [TransactionID] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NULL,
  [TransactionTime] datetime  NULL,
  [Amount] int  NULL,
  [Reason] varchar(64) COLLATE Cyrillic_General_CI_AS  NULL,
  [Previous] int  NULL
)
GO

ALTER TABLE [dbo].[DBG_GPTransactions] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_GPTransactions
-- ----------------------------
SET IDENTITY_INSERT [dbo].[DBG_GPTransactions] ON
GO

INSERT INTO [dbo].[DBG_GPTransactions] ([TransactionID], [CustomerID], [TransactionTime], [Amount], [Reason], [Previous]) VALUES (N'1', N'1000000', N'2026-06-20 03:05:11.670', N'-120', N'WZ_BuyItem_GP', N'4260')
GO

INSERT INTO [dbo].[DBG_GPTransactions] ([TransactionID], [CustomerID], [TransactionTime], [Amount], [Reason], [Previous]) VALUES (N'2', N'1000000', N'2026-06-20 03:05:18.423', N'-120', N'WZ_BuyItem_GP', N'4140')
GO

INSERT INTO [dbo].[DBG_GPTransactions] ([TransactionID], [CustomerID], [TransactionTime], [Amount], [Reason], [Previous]) VALUES (N'3', N'1000000', N'2026-06-20 03:39:23.057', N'-55', N'WZ_BuyItem_GP', N'4020')
GO

INSERT INTO [dbo].[DBG_GPTransactions] ([TransactionID], [CustomerID], [TransactionTime], [Amount], [Reason], [Previous]) VALUES (N'4', N'1000000', N'2026-06-20 03:39:31.623', N'-55', N'WZ_BuyItem_GP', N'3965')
GO

SET IDENTITY_INSERT [dbo].[DBG_GPTransactions] OFF
GO


-- ----------------------------
-- Table structure for DBG_IISApiStats
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_IISApiStats]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_IISApiStats]
GO

CREATE TABLE [dbo].[DBG_IISApiStats] (
  [API] varchar(128) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [Hits] bigint  NOT NULL,
  [BytesIn] bigint  NOT NULL,
  [BytesOut] bigint  NOT NULL
)
GO

ALTER TABLE [dbo].[DBG_IISApiStats] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_IISApiStats
-- ----------------------------

-- ----------------------------
-- Table structure for DBG_LevelUpEvents
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_LevelUpEvents]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_LevelUpEvents]
GO

CREATE TABLE [dbo].[DBG_LevelUpEvents] (
  [CustomerID] int  NULL,
  [LevelGained] int  NULL,
  [ReportTime] datetime  NULL,
  [SessionID] bigint  NOT NULL
)
GO

ALTER TABLE [dbo].[DBG_LevelUpEvents] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_LevelUpEvents
-- ----------------------------

-- ----------------------------
-- Table structure for DBG_LootRewards
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_LootRewards]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_LootRewards]
GO

CREATE TABLE [dbo].[DBG_LootRewards] (
  [RecordID] int  IDENTITY(1,1) NOT NULL,
  [ReportTime] datetime  NOT NULL,
  [CustomerID] int  NOT NULL,
  [Roll] float(53)  NOT NULL,
  [LootID] float(53)  NOT NULL,
  [ItemID] int  NOT NULL,
  [ExpDays] int  NOT NULL,
  [GD] int  NOT NULL
)
GO

ALTER TABLE [dbo].[DBG_LootRewards] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_LootRewards
-- ----------------------------
SET IDENTITY_INSERT [dbo].[DBG_LootRewards] ON
GO

SET IDENTITY_INSERT [dbo].[DBG_LootRewards] OFF
GO


-- ----------------------------
-- Table structure for DBG_PasswordResets
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_PasswordResets]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_PasswordResets]
GO

CREATE TABLE [dbo].[DBG_PasswordResets] (
  [ResetID] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NULL,
  [ResetDate] datetime  NULL,
  [NewPassword] varchar(200) COLLATE Cyrillic_General_CI_AS  NULL
)
GO

ALTER TABLE [dbo].[DBG_PasswordResets] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_PasswordResets
-- ----------------------------
SET IDENTITY_INSERT [dbo].[DBG_PasswordResets] ON
GO

SET IDENTITY_INSERT [dbo].[DBG_PasswordResets] OFF
GO


-- ----------------------------
-- Table structure for DBG_SrvLogInfo
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_SrvLogInfo]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_SrvLogInfo]
GO

CREATE TABLE [dbo].[DBG_SrvLogInfo] (
  [RecordID] int  IDENTITY(1,1) NOT NULL,
  [ReportTime] datetime  NULL,
  [IsProcessed] int  NULL,
  [CustomerID] int  NULL,
  [CharID] int DEFAULT 0 NULL,
  [CustomerIP] varchar(64) COLLATE Cyrillic_General_CI_AS  NULL,
  [GameSessionID] bigint  NULL,
  [CheatID] int  NULL,
  [RepeatCount] int  NULL,
  [Gamertag] nvarchar(64) COLLATE Cyrillic_General_CI_AS DEFAULT '' NULL,
  [Msg] varchar(512) COLLATE Cyrillic_General_CI_AS  NULL,
  [Data] varchar(4096) COLLATE Cyrillic_General_CI_AS  NULL
)
GO

ALTER TABLE [dbo].[DBG_SrvLogInfo] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_SrvLogInfo
-- ----------------------------
SET IDENTITY_INSERT [dbo].[DBG_SrvLogInfo] ON
GO

SET IDENTITY_INSERT [dbo].[DBG_SrvLogInfo] OFF
GO


-- ----------------------------
-- Table structure for DBG_UserRoundResults
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_UserRoundResults]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_UserRoundResults]
GO

CREATE TABLE [dbo].[DBG_UserRoundResults] (
  [IP] varchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [GameSessionID] bigint DEFAULT 0 NOT NULL,
  [CustomerID] int DEFAULT 0 NOT NULL,
  [GamePoints] int DEFAULT 0 NOT NULL,
  [HonorPoints] int DEFAULT 0 NOT NULL,
  [SkillPoints] int DEFAULT 0 NOT NULL,
  [Kills] int DEFAULT 0 NOT NULL,
  [Deaths] int DEFAULT 0 NOT NULL,
  [ShotsFired] int DEFAULT 0 NOT NULL,
  [ShotsHits] int  NOT NULL,
  [Headshots] int DEFAULT 0 NOT NULL,
  [AssistKills] int DEFAULT 0 NOT NULL,
  [Wins] int DEFAULT 0 NOT NULL,
  [Losses] int DEFAULT 0 NOT NULL,
  [CaptureNeutralPoints] int DEFAULT 0 NOT NULL,
  [CaptureEnemyPoints] int DEFAULT 0 NOT NULL,
  [TimePlayed] int DEFAULT 0 NOT NULL,
  [GameReportTime] datetime DEFAULT ('19700101') NOT NULL,
  [GameDollars] int DEFAULT 0 NOT NULL,
  [TeamID] int DEFAULT 2 NOT NULL,
  [MapID] int DEFAULT 255 NOT NULL,
  [MapType] int  NULL
)
GO

ALTER TABLE [dbo].[DBG_UserRoundResults] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_UserRoundResults
-- ----------------------------

-- ----------------------------
-- Table structure for DBG_WOAdminChanges
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_WOAdminChanges]') AND type IN ('U'))
	DROP TABLE [dbo].[DBG_WOAdminChanges]
GO

CREATE TABLE [dbo].[DBG_WOAdminChanges] (
  [ChangeID] int  IDENTITY(1,1) NOT NULL,
  [ChangeTime] datetime  NULL,
  [UserName] nvarchar(64) COLLATE Cyrillic_General_CI_AS  NULL,
  [Action] int  NULL,
  [Field] varchar(512) COLLATE Cyrillic_General_CI_AS  NULL,
  [RecordID] int  NULL,
  [OldValue] varchar(2048) COLLATE Cyrillic_General_CI_AS  NULL,
  [NewValue] varchar(2048) COLLATE Cyrillic_General_CI_AS  NULL
)
GO

ALTER TABLE [dbo].[DBG_WOAdminChanges] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of DBG_WOAdminChanges
-- ----------------------------
SET IDENTITY_INSERT [dbo].[DBG_WOAdminChanges] ON
GO

SET IDENTITY_INSERT [dbo].[DBG_WOAdminChanges] OFF
GO


-- ----------------------------
-- Table structure for FinancialTransactions
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[FinancialTransactions]') AND type IN ('U'))
	DROP TABLE [dbo].[FinancialTransactions]
GO

CREATE TABLE [dbo].[FinancialTransactions] (
  [CustomerID] int  NOT NULL,
  [TransactionID] varchar(64) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [TransactionType] int  NOT NULL,
  [DateTime] datetime  NOT NULL,
  [Amount] float(53)  NOT NULL,
  [ResponseCode] varchar(64) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [AprovalCode] varchar(16) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [ItemID] varchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL
)
GO

ALTER TABLE [dbo].[FinancialTransactions] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of FinancialTransactions
-- ----------------------------
INSERT INTO [dbo].[FinancialTransactions] ([CustomerID], [TransactionID], [TransactionType], [DateTime], [Amount], [ResponseCode], [AprovalCode], [ItemID]) VALUES (N'1000000', N'INGAME', N'3000', N'2026-06-20 03:05:11.670', N'120', N'1', N'APPROVED', N'101336')
GO

INSERT INTO [dbo].[FinancialTransactions] ([CustomerID], [TransactionID], [TransactionType], [DateTime], [Amount], [ResponseCode], [AprovalCode], [ItemID]) VALUES (N'1000000', N'INGAME', N'3000', N'2026-06-20 03:05:18.423', N'120', N'1', N'APPROVED', N'101336')
GO

INSERT INTO [dbo].[FinancialTransactions] ([CustomerID], [TransactionID], [TransactionType], [DateTime], [Amount], [ResponseCode], [AprovalCode], [ItemID]) VALUES (N'1000000', N'INGAME', N'3000', N'2026-06-20 03:39:23.060', N'55', N'1', N'APPROVED', N'20025')
GO

INSERT INTO [dbo].[FinancialTransactions] ([CustomerID], [TransactionID], [TransactionType], [DateTime], [Amount], [ResponseCode], [AprovalCode], [ItemID]) VALUES (N'1000000', N'INGAME', N'3000', N'2026-06-20 03:39:31.623', N'55', N'1', N'APPROVED', N'20025')
GO


-- ----------------------------
-- Table structure for FriendsMap
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[FriendsMap]') AND type IN ('U'))
	DROP TABLE [dbo].[FriendsMap]
GO

CREATE TABLE [dbo].[FriendsMap] (
  [CustomerID] int  NULL,
  [FriendID] int  NULL,
  [FriendStatus] int  NULL,
  [DateAdded] datetime  NULL
)
GO

ALTER TABLE [dbo].[FriendsMap] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of FriendsMap
-- ----------------------------

-- ----------------------------
-- Table structure for Items_Attachments
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Items_Attachments]') AND type IN ('U'))
	DROP TABLE [dbo].[Items_Attachments]
GO

CREATE TABLE [dbo].[Items_Attachments] (
  [ItemID] int  NOT NULL,
  [FNAME] varchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [Type] int  NOT NULL,
  [Name] nvarchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [Description] nvarchar(256) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [MuzzleParticle] varchar(64) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [FireSound] varchar(256) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [Damage] float(53)  NOT NULL,
  [Range] float(53)  NOT NULL,
  [Firerate] float(53)  NOT NULL,
  [Recoil] float(53)  NOT NULL,
  [Spread] float(53)  NOT NULL,
  [Clipsize] int  NOT NULL,
  [ScopeMag] float(53)  NOT NULL,
  [ScopeType] varchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [AnimPrefix] varchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [SpecID] int DEFAULT 0 NOT NULL,
  [Category] int DEFAULT 19 NOT NULL,
  [Price1] int DEFAULT 0 NOT NULL,
  [Price7] int DEFAULT 0 NOT NULL,
  [Price30] int DEFAULT 0 NOT NULL,
  [PriceP] int DEFAULT 0 NOT NULL,
  [GPrice1] int DEFAULT 0 NOT NULL,
  [GPrice7] int DEFAULT 0 NOT NULL,
  [GPrice30] int DEFAULT 0 NOT NULL,
  [GPriceP] int DEFAULT 0 NOT NULL,
  [IsNew] int DEFAULT 0 NOT NULL,
  [LevelRequired] int DEFAULT 0 NOT NULL,
  [Weight] int DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[Items_Attachments] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Items_Attachments
INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410215', N'Mag_9mm_01', N'4', N'$Mag_9mm_01', N'$Mag_9mm_01_desc', N'', N'0', N'0', N'0', N'100', N'0', N'0', N'12', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410219', N'IS_ASR_AK12', N'1', N'$IS_ASR_AK12', N'$IS_ASR_AK12_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410475', N'Muzzle_Silencer_01', N'0', N'$Muzzle_Silencer_01', N'$Muzzle_Silencer_01_desc', N'muzzle_asr_noflash', N'Sounds/Weapons/Guns/Generic/Silencer/Silencer', N'0', N'0', N'0', N'0', N'-0.05', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410476', N'Muzzle_Silencer_HG_01', N'0', N'$Muzzle_Silencer_HG_01', N'$Muzzle_Silencer_HG_01_desc', N'muzzle_asr_noflash', N'Sounds/Weapons/Guns/Generic/Silencer/Silencer', N'0', N'0', N'0', N'0', N'-0.05', N'0', N'0', N'', N'', N'2', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410491', N'Muzzle_Silencer_HG_02', N'0', N'$Muzzle_Silencer_HG_02', N'$Muzzle_Silencer_HG_02_desc', N'muzzle_asr_noflash', N'Sounds/Weapons/Guns/Generic/Silencer/Silencer', N'0', N'0', N'0', N'0', N'-0.05', N'0', N'0', N'', N'', N'2', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410498', N'Muzzle_FlashHider_01', N'0', N'$Muzzle_FlashHider_01', N'$Muzzle_FlashHider_01_desc', N'', N'0', N'0', N'0', N'0', N'-0.05', N'0', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410499', N'SR_IRLaser_02', N'2', N'$SR_IR_01', N'$SR_IR_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.02', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410500', N'SR_IRLaser_01', N'2', N'$SR_IRLaser_01', N'$SR_IRLaser_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.02', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410501', N'BR_Grip_01', N'3', N'$BR_Grip_01', N'$BR_Grip_01_desc', N'', N'0', N'0', N'0', N'0', N'-0.15', N'-0.07', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1000')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410503', N'SR_Flashlight_01', N'2', N'$SR_Flashlight_01', N'$SR_Flashlight_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.02', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410639', N'IS_ASR_AR_01', N'1', N'$IS_ASR_AR_01', N'$IS_ASR_AR_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410640', N'IS_ASR_HB', N'1', N'$IS_ASR_HB', N'$IS_ASR_HB_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410641', N'IS_ASR_HB_01', N'1', N'$IS_ASR_HB_01', N'$IS_ASR_HB_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410642', N'IS_ASR_MXZ5', N'1', N'$IS_ASR_MXZ5', N'$IS_ASR_MXZ5_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410643', N'IS_ASR_SHRAM', N'1', N'$IS_ASR_SHRAM', N'$IS_ASR_SHRAM_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410644', N'IS_HG_9mm', N'1', N'$IS_HG_9B', N'$IS_HG_9B_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410645', N'IS_HG_45c', N'1', N'$IS_HG_45c', N'$IS_HG_45c_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410646', N'IS_HG_RV50c', N'1', N'$IS_HG_RV50c', N'$IS_HG_RV50c_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410647', N'IS_LMG_HMG_01', N'1', N'$IS_LMG_HMG_01', N'$IS_LMG_HMG_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410648', N'IS_LMG_NG100', N'1', N'$IS_LMG_NG100', N'$IS_LMG_NG100_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410649', N'IS_SHG_Saiga', N'1', N'$IS_SHG_Saiga', N'$IS_SHG_Saiga_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410650', N'IS_SHG_SP12', N'1', N'$IS_SHG_SP12', N'$IS_SHG_SP12_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410651', N'IS_SMG_Bizon', N'1', N'$IS_SMG_Bizon', N'$IS_SMG_Bizon_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410652', N'IS_SMG_EVO', N'1', N'$IS_SMG_EVO', N'$IS_SMG_EVO_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410653', N'IS_SMG_HB_02', N'1', N'$IS_SMG_HB_02', N'$IS_SMG_HB_02_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410654', N'IS_SMG_HSE', N'1', N'$IS_SMG_HSE', N'$IS_SMG_HSE_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410655', N'IS_SMG_Kris', N'1', N'$IS_SMG_Kris', N'$IS_SMG_Kris_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410659', N'IS_SUP_AT4', N'1', N'$IS_SUP_AT4', N'$IS_SUP_AT4_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410661', N'IS_SUP_RPG7', N'1', N'$IS_SUP_RPG7', N'$IS_SUP_RPG7_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410662', N'Mag_45ACP_25r_01', N'4', N'$Mag_45ACP_25r_01', N'$Mag_45ACP_25r_01_desc', N'', N'0', N'0', N'0', N'101', N'0', N'0', N'12', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410663', N'Mag_45ACP_50r_01', N'4', N'$Mag_45ACP_50r_01', N'$Mag_45ACP_50r_01_desc', N'', N'0', N'0', N'0', N'101', N'0', N'0', N'20', N'0', N'', N'', N'48', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410664', N'Mag_AK_30', N'4', N'$Mag_AK_30', N'$Mag_AK_30_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410665', N'Mag_AK_45', N'4', N'$Mag_AK_45', N'$Mag_AK_45_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'45', N'0', N'', N'', N'30', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410666', N'Mag_AK_Drum', N'4', N'$Mag_AK_Drum', N'$Mag_AK_Drum_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'100', N'0', N'', N'', N'30', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410667', N'Mag_ArchAngel_01', N'4', N'$Mag_AAR_01', N'$Mag_AAR_01_desc', N'', N'0', N'0', N'0', N'500', N'0', N'0', N'3', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410668', N'Mag_ArchAngel_02', N'4', N'$Mag_AAR_02', N'$Mag_AAR_02_desc', N'', N'0', N'0', N'0', N'500', N'0', N'0', N'6', N'0', N'', N'', N'32', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410669', N'Mag_ASR_AR_01', N'4', N'$Mag_ASR_AR_01', N'$Mag_ASR_AR_01_desc', N'', N'0', N'0', N'0', N'25', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410670', N'MAG_ASR_HB_01', N'4', N'$MAG_ASR_HB_01', N'$MAG_ASR_HB_01_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410673', N'Mag_HG_RV50c', N'4', N'$Mag_HG_RV50c', N'$Mag_HG_RV50c_desc', N'', N'0', N'0', N'0', N'101', N'0', N'0', N'4', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410675', N'Mag_LMG_HMG_01', N'4', N'$Mag_LMG_HMG_01', N'$Mag_LMG_HMG_01_desc', N'', N'0', N'0', N'0', N'25', N'0', N'0', N'200', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410676', N'Mag_LMG_NG100', N'4', N'$Mag_LMG_NG100', N'$Mag_LMG_NG100_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'100', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410677', N'Mag_missile_at4_01', N'4', N'$Mag_missile_at4_01', N'$Mag_missile_at4_01_desc', N'', N'0', N'0', N'0', N'700', N'0', N'0', N'1', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410678', N'MAG_Missile_RPG7_01', N'4', N'$MAG_Missile_RPG7_01', N'$MAG_Missile_RPG7_01_desc', N'', N'0', N'0', N'0', N'900', N'0', N'0', N'1', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410679', N'MAG_Missle_Eliminator_01', N'4', N'$MAG_Missle_Eliminator_01', N'$MAG_Missle_Eliminator_01_desc', N'', N'0', N'0', N'0', N'800', N'0', N'0', N'1', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410680', N'MAG_SHG_Saiga', N'4', N'$MAG_SHG_Saiga', N'$MAG_SHG_Saiga_desc', N'', N'0', N'0', N'0', N'300', N'0', N'0', N'5', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410681', N'MAG_SHG_Saiga_High', N'4', N'$MAG_SHG_Saiga_High', N'$MAG_SHG_Saiga_High_desc', N'', N'0', N'0', N'0', N'300', N'0', N'0', N'10', N'0', N'', N'', N'88', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410682', N'Mag_SHG_SP12', N'4', N'$Mag_SHG_SP12', N'$Mag_SHG_SP12_desc', N'', N'0', N'0', N'0', N'300', N'0', N'0', N'8', N'-1', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410683', N'MAG_SMG_Bizon', N'4', N'$MAG_SMG_Bizon', N'$MAG_SMG_Bizon_desc', N'', N'0', N'0', N'0', N'100', N'0', N'0', N'64', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410684', N'MAG_SMG_EVO_01', N'4', N'$MAG_SMG_EVO_01', N'$MAG_SMG_EVO_01_desc', N'', N'0', N'0', N'0', N'101', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410685', N'MAG_SMG_EVO_02', N'4', N'$MAG_SMG_EVO_02', N'$MAG_SMG_EVO_02_desc', N'', N'0', N'0', N'0', N'101', N'0', N'0', N'60', N'0', N'', N'', N'77', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410686', N'MAG_SMG_HB_02', N'4', N'$MAG_SMG_HB_02', N'$MAG_SMG_HB_02_desc', N'', N'0', N'0', N'0', N'100', N'0', N'0', N'14', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410687', N'MAG_SMG_HSE', N'4', N'$MAG_SMG_HSE', N'$MAG_SMG_HSE_desc', N'', N'0', N'0', N'0', N'100', N'0', N'0', N'32', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410688', N'MAG_SMG_Kris', N'4', N'$MAG_SMG_Kris', N'$MAG_SMG_Kris_desc', N'', N'0', N'0', N'0', N'101', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410689', N'MAG_SNP_ARS', N'4', N'$MAG_SNP_ARS', N'$MAG_SNP_ARS_desc', N'', N'0', N'0', N'0', N'500', N'0', N'0', N'3', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410690', N'MAG_SNP_LM7', N'4', N'$MAG_SNP_LM7', N'$MAG_SNP_LM7_desc', N'', N'0', N'150', N'250', N'500', N'15', N'1', N'3', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410691', N'MAG_Stanag_30', N'4', N'$MAG_Stanag_30', N'$MAG_Stanag_30_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410692', N'MAG_Stanag_60', N'4', N'$MAG_Stanag_60', N'$MAG_Stanag_60_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'60', N'0', N'', N'', N'35', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410693', N'MAG_Stanag_Drum', N'4', N'$MAG_Stanag_Drum', N'$MAG_Stanag_Drum_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'100', N'0', N'', N'', N'35', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410694', N'Muzzle_FlashHider_HG_01', N'0', N'$Muzzle_FlashHider_HG_01', N'$Muzzle_FlashHider_HG_01_desc', N'', N'0', N'0', N'0', N'0', N'-0.05', N'0', N'0', N'0', N'', N'', N'2', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410695', N'Muzzle_Compensator_01', N'0', N'$Muzzle_Compensator_01', N'$Muzzle_Compensator_01_desc', N'muzzle_asr_noflash', N'0', N'0', N'0', N'0', N'-0.15', N'-0.05', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410696', N'Muzzle_Standard_01', N'0', N'$Muzzle_Standard_01', N'$Muzzle_Standard_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'', N'', N'999', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410697', N'Muzzle_Standard_Saiga_01', N'0', N'$Muzzle_Standard_Saiga_01', N'$Muzzle_Standard_Saiga_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'', N'', N'999', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410698', N'Optic_Acog_01', N'1', N'$Optic_Acog_01', N'$Optic_Acog_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.15', N'0', N'35', N'Scope_Acog1', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410699', N'Optic_Eliminator', N'1', N'$Optic_Eliminator', N'$Optic_Eliminator_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'5', N'sniper4_SW', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410700', N'Optic_Holographic_01', N'1', N'$Optic_Holographic_01', N'$Optic_Holographic_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.15', N'0', N'10', N'Scope_Reflex1', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410701', N'Optic_Holographic_HG_01', N'1', N'$Optic_Holographic_HG_01', N'$Optic_Holographic_HG_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.15', N'0', N'5', N'Scope_HG_Reflex', N'', N'2', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410702', N'Optic_LongRange_01', N'1', N'$Optic_LongRange_01', N'$Optic_LongRange_01_desc', N'', N'0', N'0', N'0', N'0', N'-0.4', N'-0.15', N'0', N'100', N'Scope_Sniper_LR', N'', N'3', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410703', N'Optic_MediumRange_01', N'1', N'$Optic_MediumRange_01', N'$Optic_MediumRange_01_desc', N'', N'0', N'0', N'0', N'0', N'-0.15', N'0', N'0', N'75', N'Scope_Sniper_M', N'', N'3', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410704', N'Optic_RedDot_HG_01', N'1', N'$Optic_RedDot_HG_01', N'$Optic_RedDot_HG_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.15', N'0', N'10', N'Scope_HG_Red', N'', N'2', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410706', N'SR_Flashlight_HG_01', N'2', N'$SR_Flashlight_HG_01', N'$SR_Flashlight_HG_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.02', N'0', N'0', N'', N'', N'2', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410707', N'SR_FlashlightWide_01', N'2', N'$SR_FlashlightWide_01', N'$SR_FlashlightWide_01_desc', N'Muzzle_flashlight_wide_01', N'0', N'0', N'0', N'0', N'0', N'-0.02', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410708', N'SR_Laser_01', N'2', N'$SR_Laser_01', N'$SR_Laser_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.02', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410709', N'SR_Laser_HG_01', N'2', N'$SR_Laser_HG_01', N'$SR_Laser_HG_01_desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.02', N'0', N'0', N'', N'', N'2', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410839', N'Optic_MediumRange_01', N'1', N'$Optic_MediumRange_01_ASR', N'$Optic_MediumRange_01_ASR_desc', N'', N'0', N'0', N'0', N'0', N'-0.1', N'-0.15', N'0', N'50', N'Scope_Sniper_ASR', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410840', N'Mag_SMG_ChangFeng', N'4', N'$Mag_SMG_ChangFeng', N'$Mag_SMG_ChangFeng_desc', N'', N'0', N'0', N'0', N'100', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410841', N'Mag_ASR_ASh12', N'4', N'$Mag_ASR_ASh12', N'$Mag_ASR_ASh12_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410843', N'Mag_HG_AP50', N'4', N'$Mag_HG_AP50', N'$Mag_HG_AP50_desc', N'', N'0', N'0', N'0', N'101', N'0', N'0', N'7', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410844', N'Mag_SHG_M1216', N'4', N'$Mag_SHG_M1216', N'$Mag_SHG_M1216_desc', N'', N'0', N'0', N'0', N'300', N'0', N'0', N'15', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410845', N'Mag_HG_MicroUzi', N'4', N'$Mag_HG_MicroUzi', N'$Mag_HG_MicroUzi_desc', N'', N'0', N'0', N'0', N'101', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410846', N'Mag_SNP_Bullistic', N'4', N'$Mag_SNP_Bullistic', N'$Mag_SNP_Bullistic_desc', N'', N'0', N'0', N'0', N'500', N'0', N'0', N'4', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410847', N'IS_ASR_ASh12', N'1', N'$IS_ASR_ASh12', N'$IS_ASR_ASh12_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410848', N'IS_SHG_M1216', N'1', N'$IS_SHG_M1216', N'$IS_SHG_M1216_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410849', N'IS_SMG_ChangFeng', N'1', N'$IS_SMG_ChangFeng', N'$IS_SMG_ChangFeng_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410850', N'IS_HG_AP50', N'1', N'$IS_HG_AP50', N'$IS_HG_AP50_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410851', N'IS_HG_MicroUzi', N'1', N'$IS_HG_MicroUzi', N'$IS_HG_MicroUzi_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410852', N'Optic_RedDot_M4S', N'1', N'$Optic_RedDot_M4S', N'$Optic_RedDot_M4S_Desc', N'', N'0', N'0', N'0', N'0', N'0', N'-0.15', N'0', N'10', N'Scope_Red1', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'500')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410853', N'IS_ASR_Tar30', N'1', N'$IS_ASR_Tar30', N'$IS_ASR_Tar30_desc', N'', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'default', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410854', N'Mag_ASR_Tar30', N'4', N'$Mag_ASR_Tar30', N'$Mag_ASR_Tar30_desc', N'', N'0', N'0', N'0', N'110', N'0', N'0', N'30', N'0', N'', N'', N'0', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'250')

INSERT INTO [dbo].[Items_Attachments] ([ItemID], [FNAME], [Type], [Name], [Description], [MuzzleParticle], [FireSound], [Damage], [Range], [Firerate], [Recoil], [Spread], [Clipsize], [ScopeMag], [ScopeType], [AnimPrefix], [SpecID], [Category], [Price1], [Price7], [Price30], [PriceP], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [IsNew], [LevelRequired], [Weight]) VALUES (N'410874', N'BR_Grip_02', N'3', N'$BR_Grip_02', N'$BR_Grip_02_desc', N'', N'0', N'0', N'0', N'0', N'-0.07', N'-0.15', N'0', N'0', N'', N'', N'1', N'19', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1000')

-- Table structure for Items_Generic
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Items_Generic]') AND type IN ('U'))
	DROP TABLE [dbo].[Items_Generic]
GO

CREATE TABLE [dbo].[Items_Generic] (
  [ItemID] int  NOT NULL,
  [FNAME] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT 'Item_Generic' NOT NULL,
  [Category] int  NOT NULL,
  [Name] nvarchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [Description] nvarchar(512) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [Price1] int DEFAULT 0 NOT NULL,
  [Price7] int DEFAULT 0 NOT NULL,
  [Price30] int DEFAULT 0 NOT NULL,
  [PriceP] int DEFAULT 0 NOT NULL,
  [IsNew] int DEFAULT 0 NOT NULL,
  [LevelRequired] int DEFAULT 0 NOT NULL,
  [GPrice1] int DEFAULT 0 NOT NULL,
  [GPrice7] int DEFAULT 0 NOT NULL,
  [GPrice30] int DEFAULT 0 NOT NULL,
  [GPriceP] int DEFAULT 0 NOT NULL,
  [Weight] int DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[Items_Generic] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Items_Generic
-- ----------------------------
INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301151', N'Account_ClanCreate', N'1', N'Account_ClanCreate', N'clan create item', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301152', N'Account_ClanUpg1', N'1', N'Account_ClanUpg1', N'buy price is in permanent GC ($)
NOTE- number of added clan members is in **permanent GD** price', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301153', N'Account_ClanUpg2', N'1', N'Account_ClanUpg2', N'buy price is in permanent GC ($)
NOTE- number of added clan members is in **permanent GD** price', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301154', N'Account_ClanUpg3', N'1', N'Account_ClanUpg3', N'buy price is in permanent GC ($)
NOTE- number of added clan members is in **permanent GD** price', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301155', N'Account_ClanUpg4', N'1', N'Account_ClanUpg4', N'buy price is in permanent GC ($)
NOTE- number of added clan members is in **permanent GD** price', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301156', N'Account_ClanUpg5', N'1', N'Account_ClanUpg5', N'buy price is in permanent GC ($)
NOTE- number of added clan members is in **permanent GD** price', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301157', N'Account_ClanUpg6', N'1', N'Account_ClanUpg6', N'buy price is in permanent GC ($)
NOTE- number of added clan members is in **permanent GD** price', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301159', N'Char_Revive', N'1', N'Char_Revive', N'item for char revive before time, price is permanent GC', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301257', N'Account_PremoumSubscribe', N'1', N'Premium Subscription', N'one month subscription for premium servers', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'301399', N'Char_NameChange', N'1', N'Char_NameChange', N'item for character renaming', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'462901', N'$Pistol_Ammo9mm', N'40', N'$AmmoBox_HG_SMG_HP', N'$AmmoBox_HG_SMG_HP_desc', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'5')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'462904', N'$Pistol_Ammo45', N'40', N'$AmmoBox_HG_HP', N'$AmmoBox_HG_HP_desc', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'5')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'462906', N'$Pistol_Ammo50', N'40', N'$AmmoBox_HG_AP', N'$AmmoBox_HG_AP_desc', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'5')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'462908', N'$ASR_Ammo', N'40', N'$AmmoBox_ASR_HP', N'$AmmoBox_ASR_HP_desc', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'5')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'462914', N'$Shotgun_Ammo', N'40', N'$AmmoBox_ShotGun_Buckshot', N'$AmmoBox_ShotGun_Buckshot_desc', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'5')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'462926', N'$Sniper_Ammo', N'40', N'$SniperRound_MLR', N'$SniperRound_MLR_desc', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'5')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'462936', N'$AT4_Ammo', N'40', N'$AT4_Rocket_Base', N'$AT4_Rocket_Base_Desc', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'5')
GO

INSERT INTO [dbo].[Items_Generic] ([ItemID], [FNAME], [Category], [Name], [Description], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [Weight]) VALUES (N'462944', N'$RPG7_Ammo', N'40', N'$RPG7_Rocket_Base', N'$RPG7_Rocket_Base_Desc', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'5')
GO

-- ----------------------------
-- Table structure for Items_LootData
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Items_LootData]') AND type IN ('U'))
	DROP TABLE [dbo].[Items_LootData]
GO

CREATE TABLE [dbo].[Items_LootData] (
  [RecordID] int  IDENTITY(1,1) NOT NULL,
  [LootID] int  NOT NULL,
  [Chance] float(53)  NULL,
  [ItemID] int  NULL,
  [GDMin] int  NULL,
  [GDMax] int  NULL
)
GO

ALTER TABLE [dbo].[Items_LootData] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Items_LootData
-- ----------------------------
SET IDENTITY_INSERT [dbo].[Items_LootData] ON
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'1', N'301118', N'20', N'101267', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'2', N'301118', N'40', N'101309', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'3', N'301118', N'50', N'101308', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'4', N'301118', N'300', N'101307', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'5', N'301118', N'500', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'6', N'301118', N'900', N'101278', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'7', N'301119', N'1', N'101032', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'8', N'301119', N'1', N'101098', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'9', N'301119', N'2', N'101111', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'10', N'301119', N'5', N'101040', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'11', N'301119', N'10', N'101158', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'12', N'301119', N'10', N'101115', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'13', N'301119', N'100', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'14', N'301120', N'1', N'101172', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'15', N'301120', N'1', N'101247', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'16', N'301120', N'1', N'101103', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'17', N'301120', N'5', N'101022', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'18', N'301120', N'10', N'101200', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'19', N'301120', N'20', N'101002', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'20', N'301120', N'30', N'101111', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'21', N'301120', N'100', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'22', N'301121', N'1', N'101088', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'23', N'301121', N'5', N'101063', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'24', N'301121', N'5', N'101103', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'25', N'301121', N'5', N'101022', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'26', N'301121', N'10', N'101064', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'27', N'301121', N'10', N'101112', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'28', N'301121', N'20', N'101055', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'29', N'301121', N'40', N'101002', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'30', N'301121', N'40', N'101120', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'31', N'301121', N'40', N'101004', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'32', N'301121', N'400', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'33', N'301122', N'1', N'400043', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'34', N'301122', N'4', N'400070', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'35', N'301122', N'10', N'400017', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'36', N'301122', N'30', N'400048', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'37', N'301122', N'60', N'400073', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'38', N'301122', N'70', N'400101', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'39', N'301122', N'70', N'400016', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'40', N'301122', N'100', N'400136', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'41', N'301122', N'100', N'400137', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'42', N'301122', N'200', N'400071', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'43', N'301122', N'400', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'44', N'301123', N'100', N'400000', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'45', N'301123', N'2000', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'46', N'301124', N'10', N'20056', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'47', N'301124', N'10', N'20015', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'48', N'301124', N'60', N'20043', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'49', N'301124', N'60', N'20047', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'50', N'301124', N'60', N'20048', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'51', N'301124', N'100', N'20096', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'52', N'301124', N'100', N'20097', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'53', N'301124', N'100', N'20098', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'54', N'301124', N'100', N'20177', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'55', N'301124', N'100', N'20178', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'56', N'301124', N'100', N'20023', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'57', N'301124', N'100', N'20032', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'58', N'301124', N'100', N'20035', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'59', N'301124', N'100', N'20041', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'60', N'301124', N'100', N'20042', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'61', N'301124', N'1000', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'62', N'301125', N'5', N'109506', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'63', N'301125', N'7', N'101302', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'64', N'301125', N'10', N'109505', N'0', N'0')
GO

GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'66', N'301125', N'60', N'109504', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'67', N'301125', N'80', N'109505', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'68', N'301125', N'200', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'69', N'301126', N'22', N'101293', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'70', N'301126', N'24', N'101292', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'71', N'301126', N'26', N'101291', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'72', N'301126', N'30', N'101290', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'73', N'301126', N'50', N'101297', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'74', N'301126', N'50', N'101298', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'75', N'301126', N'50', N'101286', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'76', N'301126', N'50', N'101294', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'77', N'301126', N'100', N'101283', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'78', N'301126', N'100', N'101285', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'79', N'301126', N'100', N'101288', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'80', N'301126', N'100', N'101289', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'81', N'301126', N'110', N'101296', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'82', N'301126', N'400', N'101299', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'83', N'301126', N'600', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'84', N'301127', N'10', N'101295', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'85', N'301127', N'20', N'101286', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'86', N'301127', N'50', N'101298', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'87', N'301127', N'60', N'101297', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'88', N'301127', N'100', N'101290', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'89', N'301127', N'100', N'101291', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'90', N'301127', N'100', N'101292', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'91', N'301127', N'100', N'101293', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'92', N'301127', N'100', N'101299', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'93', N'301127', N'150', N'101285', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'94', N'301127', N'2000', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'95', N'301128', N'100', N'101284', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'96', N'301128', N'100', N'101295', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'97', N'301128', N'2000', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'98', N'301129', N'1', N'400070', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'99', N'301129', N'10', N'400050', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'100', N'301129', N'10', N'400079', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'101', N'301129', N'10', N'400017', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'102', N'301129', N'25', N'400015', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'103', N'301129', N'30', N'400016', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'104', N'301129', N'100', N'400071', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'105', N'301129', N'200', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'106', N'301130', N'10', N'301118', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'107', N'301130', N'10', N'301137', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'108', N'301130', N'30', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'109', N'301130', N'100', N'301126', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'110', N'301132', N'5', N'301119', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'111', N'301132', N'5', N'301122', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'112', N'301132', N'10', N'301137', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'113', N'301132', N'20', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'114', N'301132', N'100', N'301126', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'115', N'301133', N'10', N'301119', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'116', N'301133', N'10', N'301122', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'117', N'301133', N'10', N'301124', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'118', N'301133', N'30', N'301126', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'119', N'301133', N'30', N'301137', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'120', N'301133', N'100', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'121', N'301134', N'20', N'301120', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'122', N'301134', N'20', N'301129', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'123', N'301134', N'60', N'301128', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'124', N'301134', N'100', N'301124', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'125', N'301134', N'100', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'126', N'301135', N'10', N'301123', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'127', N'301135', N'20', N'301121', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'128', N'301135', N'100', N'301124', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'129', N'301135', N'100', N'301127', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'130', N'301135', N'100', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'131', N'301136', N'20', N'0', N'50', N'100')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'132', N'301136', N'40', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'133', N'301137', N'1', N'112995', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'134', N'301137', N'1', N'112999', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'135', N'301137', N'20', N'20179', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'136', N'301137', N'40', N'20175', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'137', N'301137', N'70', N'101307', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'138', N'301137', N'100', N'101315', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'139', N'301137', N'100', N'101311', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'140', N'301137', N'100', N'101312', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'141', N'301137', N'100', N'101306', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'142', N'301137', N'100', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'1', N'301118', N'20', N'101267', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'2', N'301118', N'40', N'101309', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'3', N'301118', N'50', N'101308', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'4', N'301118', N'300', N'101307', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'5', N'301118', N'500', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'6', N'301118', N'900', N'101278', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'7', N'301119', N'1', N'101032', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'8', N'301119', N'1', N'101098', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'9', N'301119', N'2', N'101111', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'10', N'301119', N'5', N'101040', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'11', N'301119', N'10', N'101158', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'12', N'301119', N'10', N'101115', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'13', N'301119', N'100', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'14', N'301120', N'1', N'101172', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'15', N'301120', N'1', N'101247', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'16', N'301120', N'1', N'101103', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'17', N'301120', N'5', N'101022', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'18', N'301120', N'10', N'101200', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'19', N'301120', N'20', N'101002', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'20', N'301120', N'30', N'101111', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'21', N'301120', N'100', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'22', N'301121', N'1', N'101088', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'23', N'301121', N'5', N'101063', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'24', N'301121', N'5', N'101103', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'25', N'301121', N'5', N'101022', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'26', N'301121', N'10', N'101064', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'27', N'301121', N'10', N'101112', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'28', N'301121', N'20', N'101055', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'29', N'301121', N'40', N'101002', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'30', N'301121', N'40', N'101120', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'31', N'301121', N'40', N'101004', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'32', N'301121', N'400', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'33', N'301122', N'1', N'400043', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'34', N'301122', N'4', N'400070', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'35', N'301122', N'10', N'400017', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'36', N'301122', N'30', N'400048', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'37', N'301122', N'60', N'400073', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'38', N'301122', N'70', N'400101', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'39', N'301122', N'70', N'400016', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'40', N'301122', N'100', N'400136', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'41', N'301122', N'100', N'400137', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'42', N'301122', N'200', N'400071', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'43', N'301122', N'400', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'44', N'301123', N'100', N'400000', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'45', N'301123', N'2000', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'46', N'301124', N'10', N'20056', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'47', N'301124', N'10', N'20015', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'48', N'301124', N'60', N'20043', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'49', N'301124', N'60', N'20047', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'50', N'301124', N'60', N'20048', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'51', N'301124', N'100', N'20096', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'52', N'301124', N'100', N'20097', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'53', N'301124', N'100', N'20098', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'54', N'301124', N'100', N'20177', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'55', N'301124', N'100', N'20178', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'56', N'301124', N'100', N'20023', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'57', N'301124', N'100', N'20032', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'58', N'301124', N'100', N'20035', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'59', N'301124', N'100', N'20041', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'60', N'301124', N'100', N'20042', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'61', N'301124', N'1000', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'62', N'301125', N'5', N'109506', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'63', N'301125', N'7', N'101302', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'64', N'301125', N'10', N'109505', N'0', N'0')
GO

GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'66', N'301125', N'60', N'109504', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'67', N'301125', N'80', N'109505', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'68', N'301125', N'200', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'69', N'301126', N'22', N'101293', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'70', N'301126', N'24', N'101292', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'71', N'301126', N'26', N'101291', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'72', N'301126', N'30', N'101290', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'73', N'301126', N'50', N'101297', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'74', N'301126', N'50', N'101298', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'75', N'301126', N'50', N'101286', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'76', N'301126', N'50', N'101294', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'77', N'301126', N'100', N'101283', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'78', N'301126', N'100', N'101285', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'79', N'301126', N'100', N'101288', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'80', N'301126', N'100', N'101289', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'81', N'301126', N'110', N'101296', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'82', N'301126', N'400', N'101299', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'83', N'301126', N'600', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'84', N'301127', N'10', N'101295', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'85', N'301127', N'20', N'101286', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'86', N'301127', N'50', N'101298', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'87', N'301127', N'60', N'101297', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'88', N'301127', N'100', N'101290', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'89', N'301127', N'100', N'101291', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'90', N'301127', N'100', N'101292', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'91', N'301127', N'100', N'101293', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'92', N'301127', N'100', N'101299', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'93', N'301127', N'150', N'101285', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'94', N'301127', N'2000', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'95', N'301128', N'100', N'101284', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'96', N'301128', N'100', N'101295', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'97', N'301128', N'2000', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'98', N'301129', N'1', N'400070', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'99', N'301129', N'10', N'400050', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'100', N'301129', N'10', N'400079', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'101', N'301129', N'10', N'400017', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'102', N'301129', N'25', N'400015', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'103', N'301129', N'30', N'400016', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'104', N'301129', N'100', N'400071', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'105', N'301129', N'200', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'106', N'301130', N'10', N'301118', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'107', N'301130', N'10', N'301137', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'108', N'301130', N'30', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'109', N'301130', N'100', N'301126', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'110', N'301132', N'5', N'301119', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'111', N'301132', N'5', N'301122', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'112', N'301132', N'10', N'301137', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'113', N'301132', N'20', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'114', N'301132', N'100', N'301126', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'115', N'301133', N'10', N'301119', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'116', N'301133', N'10', N'301122', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'117', N'301133', N'10', N'301124', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'118', N'301133', N'30', N'301126', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'119', N'301133', N'30', N'301137', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'120', N'301133', N'100', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'121', N'301134', N'20', N'301120', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'122', N'301134', N'20', N'301129', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'123', N'301134', N'60', N'301128', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'124', N'301134', N'100', N'301124', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'125', N'301134', N'100', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'126', N'301135', N'10', N'301123', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'127', N'301135', N'20', N'301121', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'128', N'301135', N'100', N'301124', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'129', N'301135', N'100', N'301127', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'130', N'301135', N'100', N'301125', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'131', N'301136', N'20', N'0', N'50', N'100')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'132', N'301136', N'40', N'-1', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'133', N'301137', N'1', N'112995', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'134', N'301137', N'1', N'112999', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'135', N'301137', N'20', N'20179', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'136', N'301137', N'40', N'20175', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'137', N'301137', N'70', N'101307', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'138', N'301137', N'100', N'101315', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'139', N'301137', N'100', N'101311', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'140', N'301137', N'100', N'101312', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'141', N'301137', N'100', N'101306', N'0', N'0')
GO

INSERT INTO [dbo].[Items_LootData] ([RecordID], [LootID], [Chance], [ItemID], [GDMin], [GDMax]) VALUES (N'142', N'301137', N'100', N'-1', N'0', N'0')
GO

SET IDENTITY_INSERT [dbo].[Items_LootData] OFF
GO


-- ----------------------------
-- Table structure for Items_Weapons
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Items_Weapons]') AND type IN ('U'))
	DROP TABLE [dbo].[Items_Weapons]
GO

CREATE TABLE [dbo].[Items_Weapons] (
  [ItemID] int  NOT NULL,
  [FNAME] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT 'ITEM000' NOT NULL,
  [Category] int DEFAULT 0 NOT NULL,
  [Name] nvarchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [Description] nvarchar(256) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [MuzzleOffset] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT '0 0 0' NOT NULL,
  [MuzzleParticle] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT 'default' NOT NULL,
  [Animation] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT 'assault' NOT NULL,
  [BulletID] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT 5.45 NOT NULL,
  [Sound_Shot] varchar(255) COLLATE Cyrillic_General_CI_AS DEFAULT 'Sounds/Weapons/AK74_7_62_Shot' NOT NULL,
  [Sound_Reload] varchar(255) COLLATE Cyrillic_General_CI_AS DEFAULT 'Sounds/Weapons/AK74_Reload' NOT NULL,
  [Damage] float(53) DEFAULT 20 NOT NULL,
  [isImmediate] int DEFAULT 1 NOT NULL,
  [Mass] float(53) DEFAULT 0.1 NOT NULL,
  [Speed] float(53) DEFAULT 300 NOT NULL,
  [DamageDecay] float(53) DEFAULT 0 NOT NULL,
  [Area] float(53) DEFAULT 0 NOT NULL,
  [Delay] float(53) DEFAULT 0 NOT NULL,
  [Timeout] float(53) DEFAULT 0 NOT NULL,
  [NumClips] int DEFAULT 10 NOT NULL,
  [Clipsize] int DEFAULT 30 NOT NULL,
  [ReloadTime] float(53) DEFAULT 2.5 NOT NULL,
  [ActiveReloadTick] float(53) DEFAULT 1.2 NOT NULL,
  [RateOfFire] int DEFAULT 600 NOT NULL,
  [Spread] float(53) DEFAULT 0.08 NOT NULL,
  [Recoil] float(53) DEFAULT 1 NOT NULL,
  [NumGrenades] int DEFAULT 0 NOT NULL,
  [GrenadeName] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT 'asr_grenade' NOT NULL,
  [Firemode] varchar(3) COLLATE Cyrillic_General_CI_AS DEFAULT 101 NOT NULL,
  [DetectionRadius] int DEFAULT 30 NOT NULL,
  [ScopeType] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT 'default' NOT NULL,
  [ScopeZoom] int DEFAULT 0 NOT NULL,
  [Price1] int DEFAULT 0 NOT NULL,
  [Price7] int DEFAULT 0 NOT NULL,
  [Price30] int DEFAULT 0 NOT NULL,
  [PriceP] int DEFAULT 0 NOT NULL,
  [IsNew] int DEFAULT 0 NOT NULL,
  [LevelRequired] int DEFAULT 0 NOT NULL,
  [GPrice1] int DEFAULT 0 NOT NULL,
  [GPrice7] int DEFAULT 0 NOT NULL,
  [GPrice30] int DEFAULT 0 NOT NULL,
  [GPriceP] int DEFAULT 0 NOT NULL,
  [ShotsFired] bigint DEFAULT 0 NOT NULL,
  [ShotsHits] bigint DEFAULT 0 NOT NULL,
  [KillsCQ] int DEFAULT 0 NOT NULL,
  [KillsDM] int DEFAULT 0 NOT NULL,
  [KillsSB] int DEFAULT 0 NOT NULL,
  [IsUpgradeable] int DEFAULT 1 NOT NULL,
  [IsFPS] int DEFAULT 0 NOT NULL,
  [FPSSpec0] int DEFAULT 0 NOT NULL,
  [FPSSpec1] int DEFAULT 0 NOT NULL,
  [FPSSpec2] int DEFAULT 0 NOT NULL,
  [FPSSpec3] int DEFAULT 0 NOT NULL,
  [FPSSpec4] int DEFAULT 0 NOT NULL,
  [FPSSpec5] int DEFAULT 0 NOT NULL,
  [FPSSpec6] int DEFAULT 0 NOT NULL,
  [FPSSpec7] int DEFAULT 0 NOT NULL,
  [FPSSpec8] int DEFAULT 0 NOT NULL,
  [FPSAttach0] int DEFAULT 0 NOT NULL,
  [FPSAttach1] int DEFAULT 0 NOT NULL,
  [FPSAttach2] int DEFAULT 0 NOT NULL,
  [FPSAttach3] int DEFAULT 0 NOT NULL,
  [FPSAttach4] int DEFAULT 0 NOT NULL,
  [FPSAttach5] int DEFAULT 0 NOT NULL,
  [FPSAttach6] int DEFAULT 0 NOT NULL,
  [FPSAttach7] int DEFAULT 0 NOT NULL,
  [FPSAttach8] int DEFAULT 0 NOT NULL,
  [AnimPrefix] varchar(32) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [Weight] int DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[Items_Weapons] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Items_Weapons
INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'109503', N'MedKit_Military', N'28', N'MedKit_Military', N'$MedKit_Military_desc', N'0 0 0', N'muzzle_asr', N'assault', N'5.45', N'', N'', N'95', N'1', N'10', N'300', N'50', N'0', N'0', N'0', N'1', N'3', N'4', N'2', N'30', N'20', N'20', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'0', N'4', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'', N'250')

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'109504', N'SS_Med_Antibiotics_01', N'28', N'SS_Med_Antibiotics_01', N'$SS_Med_Antibiotics_01_desc', N'0 0 0', N'muzzle_asr', N'assault', N'5.45', N'', N'', N'45', N'1', N'10', N'300', N'50', N'0', N'0', N'0', N'1', N'3', N'4', N'2', N'30', N'20', N'20', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'0', N'4', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'', N'250')

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'109505', N'SS_Med_Bandage_Fac_01', N'28', N'SS_Med_Bandage_Fac_01', N'$SS_Med_Bandage_Fac_01_desc', N'0 0 0', N'muzzle_asr', N'assault', N'5.45', N'', N'', N'45', N'1', N'10', N'300', N'50', N'0', N'0', N'0', N'1', N'3', N'4', N'2', N'30', N'20', N'20', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'0', N'4', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'', N'250')

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'109506', N'MedKit_Civilian', N'28', N'$MedKit_Civilian', N'$MedKit_Civilian_desc', N'0 0 0', N'muzzle_asr', N'assault', N'5.45', N'', N'', N'75', N'1', N'10', N'300', N'50', N'0', N'0', N'0', N'1', N'3', N'4', N'2', N'30', N'20', N'20', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'0', N'4', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'', N'250')

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112993', N'SS_Barricade_Shield_Metal_01', N'28', N'$Barricade_Metal_Shield', N'$Barricade_Metal_Shield_desc', N'0 0 0', N'muzzle_asr', N'assault', N'melee', N'', N'', N'400', N'1', N'10', N'60', N'1', N'0', N'0', N'0', N'1', N'5', N'1', N'0', N'60', N'1', N'1', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'SS_Barricade_Shield_Metal_01', N'1000')

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112995', N'SS_Barricade_BarbWire', N'28', N'$Barricade_BarbWire', N'$Barricade_BarbWire_desc', N'0 0 0', N'muzzle_asr', N'assault', N'melee', N'', N'', N'400', N'1', N'10', N'60', N'1', N'0', N'0', N'0', N'1', N'5', N'1', N'0', N'60', N'1', N'1', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'SS_Barricade_Shield_wood_01', N'500')

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112999', N'SS_Barricade_Shield_Wood_01', N'28', N'$Barricade_Wood_Shield', N'$Barricade_Wood_Shield_desc', N'0 0 0', N'muzzle_asr', N'assault', N'melee', N'', N'', N'400', N'1', N'10', N'60', N'1', N'0', N'0', N'0', N'1', N'5', N'1', N'0', N'60', N'1', N'1', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'SS_Barricade_Shield_wood_01', N'500')

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113008', N'SS_Barricade_Spike_Mat', N'28', N'$Barricade_Spike_Mat', N'$Barricade_Spike_Mat_desc', N'0 0 0', N'muzzle_asr', N'', N'ClaymoreMine', N'', N'', N'1.5', N'1', N'200', N'0.5', N'0', N'11', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'11', N'0', N'', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'', N'500')

-- ----------------------------
INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'103110', N'SNP_LM7_Mk6_1', N'21', N'$SNP_LM7_Mk6_1', N'$SNP_LM7_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/LM7/LM7_A', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'150', N'0', N'1.3', N'800', N'600', N'0', N'0', N'0', N'0', N'1', N'3', N'0.43', N'4', N'3', N'28.35', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410702', N'0', N'0', N'410690', N'0', N'0', N'0', N'0', N'SNP_LM7', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'111398', N'MEL_UNARMED', N'29', N'$MEL_UNARMED', N'$MEL_UNARMED_desc', N'0 0 0', N'', N'melee', N'melee', N'Sounds/Weapons/Melee/Fists/Melee', N'', N'7', N'1', N'10', N'521', N'1', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'100', N'6', N'12', N'0', N'asr_grenade', N'001', N'15', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'MEL_UNARMED', N'0')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112461', N'HG_9B_Tier1', N'25', N'$HG_9B_Mk1', N'$HG_9B_Mk1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_9mm/9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_9mm/HG_9mm_Reload', N'23', N'0', N'2.4', N'351', N'32', N'0', N'0', N'0', N'0', N'1', N'3.1', N'1.2', N'221', N'3.25', N'15.11', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410644', N'0', N'0', N'410215', N'0', N'0', N'0', N'0', N'HG_9mm', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112463', N'SNP_ARS_Tier2', N'21', N'$SNP_ARS_Mk4', N'$SNP_ARS_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/ARS', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'90', N'0', N'2.4', N'250', N'350', N'0', N'0', N'0', N'0', N'1', N'4.76', N'2.8', N'8', N'5.5', N'26.25', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'2018', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410689', N'0', N'0', N'0', N'0', N'SNP_ARS', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112816', N'SHG_M1216_Tier2', N'22', N'$SHG_M1216_Mk3', N'$SHG_M1216_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/M1216/M1216', N'Sounds/Weapons/Guns/Shotguns/M1216/M1216_Reload', N'30', N'0', N'1', N'251', N'8', N'0', N'0', N'0', N'0', N'1', N'3', N'3.2', N'23', N'7.4', N'16.61', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410848', N'0', N'0', N'410844', N'0', N'0', N'0', N'0', N'SHG_M1216', N'3000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112817', N'SMG_Bizon_Tier1', N'26', N'$SMG_Bizon_Mk1', N'$SMG_Bizon_Mk1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/BIZON_mod', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/SMGBizonReload', N'23', N'0', N'1', N'350', N'35', N'0', N'0', N'0', N'0', N'1', N'4.6', N'0.43', N'391', N'5', N'15.25', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410651', N'0', N'0', N'410683', N'0', N'0', N'0', N'0', N'SMG_Bizon', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112891', N'ASR_AK12_Tier1', N'20', N'$ASR_AK12_Mk1', N'$ASR_AK12_Mk1_Desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/AK12/AK12', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'371', N'65', N'0', N'0', N'0', N'0', N'1', N'3.7', N'0.43', N'341', N'6.4', N'18', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'30', N'0', N'0', N'0', N'0', N'410696', N'410219', N'0', N'0', N'410664', N'0', N'0', N'0', N'0', N'ASR_AK12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112949', N'ASR_ASh12_Tier2', N'20', N'$ASR_ASh12_Mk3', N'$ASR_ASh12_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/Ash12/Ash12', N'Sounds/Weapons/Guns/Assault Rifles/Ash12/Ash12_Reload', N'20.5', N'0', N'1', N'321', N'45', N'0', N'0', N'0', N'0', N'1', N'2.93', N'2.43', N'291', N'5.4', N'16.25', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410847', N'0', N'0', N'410841', N'0', N'0', N'0', N'0', N'ASR_ASh12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112950', N'ASR_WDX_Tier1', N'20', N'$ASR_WDX_Mk1', N'$ASR_WDX_Mk1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HB_01_Wolf/HB_Wolf', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'290', N'65', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'220', N'3', N'14.72', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'410696', N'410641', N'0', N'0', N'410670', N'0', N'0', N'0', N'0', N'ASR_HB_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112951', N'ASR_HB_Tier1', N'20', N'$ASR_HB_Mk1', N'$ASR_HB_Mk1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'25', N'0', N'1', N'341', N'80', N'0', N'0', N'0', N'0', N'1', N'3.9', N'1.43', N'381', N'3.7', N'14.71', N'0', N'asr_grenade', N'001', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410640', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_HB', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112954', N'HG_45APC_Tier1', N'25', N'$HG_45c_Mk1', N'$HG_45c_Mk1_desc', N'0 0 0', N'muzzle_asr_basic_01', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/45c', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/HG_45c_Reload', N'22', N'0', N'1', N'351', N'27', N'0', N'0', N'0', N'0', N'1', N'3.1', N'1.2', N'330', N'4', N'11.25', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'48', N'0', N'0', N'0', N'0', N'0', N'410645', N'0', N'0', N'410662', N'0', N'0', N'0', N'0', N'HG_45c', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112955', N'HG_AP50_Tier2', N'25', N'$HG_AP50_Mk4', N'$HG_AP50_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo50', N'Sounds/Weapons/Guns/Pistols/AP50/AP50', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/HG_45c_Reload', N'26', N'0', N'1', N'351', N'25', N'0', N'0', N'0', N'0', N'1', N'2.2', N'1.2', N'120', N'3.5', N'7.5', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410850', N'0', N'0', N'410843', N'0', N'0', N'0', N'0', N'HG_AP50', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112956', N'PDW_HB2_Tier1', N'25', N'$SMG_HB_02_Mk2', N'$SMG_HB_02_Mk2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/HB_02_SMG/HB_02', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/UziReload', N'21', N'0', N'1', N'310', N'33', N'0', N'0', N'0', N'0', N'1', N'3.01', N'1.2', N'281', N'4', N'15.1', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410653', N'0', N'0', N'410686', N'0', N'0', N'0', N'0', N'SMG_HB_02', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112957', N'HG_RV50_Tier2', N'25', N'$HG_RV50_Mk4', N'$HG_RV50_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo50', N'Sounds/Weapons/Guns/Pistols/RV50c/RV50c', N'Sounds/Weapons/Guns/Pistols/RV50c/RV50c_Reload', N'37', N'0', N'1', N'371', N'25', N'0', N'0', N'0', N'0', N'1', N'4.14', N'2.2', N'90', N'4.5', N'29.15', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410646', N'0', N'0', N'410673', N'0', N'0', N'0', N'0', N'HG_RV50c', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112958', N'LMG_HMG_Mk6_1', N'23', N'$LMG_HMG_01_Mk6_1', N'$LMG_HMG_01_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo50', N'Sounds/Weapons/Guns/LargeMachine Guns/HMG_01/HMG_01', N'Sounds/Weapons/Guns/LargeMachine Guns/HMG_01/HMG_01_Reload', N'35', N'0', N'1', N'500', N'87', N'0', N'0', N'0', N'0', N'1', N'6.24', N'4.1', N'421', N'4.5', N'20', N'0', N'asr_grenade', N'001', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410647', N'0', N'0', N'410675', N'0', N'0', N'0', N'0', N'LMG_HMG_01', N'8000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112959', N'LMG_NG100_Tier2', N'23', N'$LMG_NG100_Mk3', N'$LMG_NG100_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/LargeMachine Guns/NG100/NG100', N'Sounds/Weapons/Guns/LargeMachine Guns/NG100/LMGNG100Reload_v2', N'33', N'0', N'1', N'600', N'41', N'0', N'0', N'0', N'0', N'1', N'6.24', N'2.43', N'341', N'5.1', N'17', N'0', N'asr_grenade', N'001', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410648', N'0', N'0', N'410676', N'0', N'0', N'0', N'0', N'LMG_NG100', N'5000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112960', N'SHG_Saiga_Tier1', N'22', N'$SHG_Saiga_Mk1', N'$SHG_Saiga_Mk1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/Saiga/Saiga', N'Sounds/Weapons/Guns/Shotguns/Saiga/SaigaReload', N'25.5', N'0', N'1', N'221', N'6', N'0', N'0', N'0', N'0', N'1', N'3.92', N'1.2', N'40', N'7.4', N'18.61', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'88', N'0', N'0', N'0', N'0', N'410697', N'410649', N'0', N'0', N'410680', N'0', N'0', N'0', N'0', N'SHG_Saiga', N'3400')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112961', N'SMG_CF10_Tier2', N'26', N'$SMG_CF-10_Mk3', N'$SMG_CF-10_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/ChangFeng/ChangFeng', N'Sounds/Weapons/Guns/SubMachine Guns/ChangFeng/SMGBizonReload', N'21', N'0', N'1', N'341', N'24', N'0', N'0', N'0', N'0', N'1', N'3.51', N'0.43', N'271', N'6.65', N'15.15', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410849', N'0', N'0', N'410840', N'0', N'0', N'0', N'0', N'SMG_ChangFeng', N'2500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112962', N'SMG_EVO_Tier1', N'26', N'$SMG_EVO_Mk1', N'$SMG_EVO_Mk1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/EVO_mod', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/SMBEvoReload', N'24', N'0', N'1', N'410', N'35', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'321', N'4.7', N'19.1', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'77', N'0', N'0', N'0', N'0', N'410696', N'410652', N'0', N'0', N'410684', N'0', N'0', N'0', N'0', N'SMG_EVO', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112963', N'SMG_Kris_Tier2', N'26', N'$SMG_Kris_Mk4', N'$SMG_Kris_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo45', N'Sounds/Weapons/Guns/SubMachine Guns/Kris/KRIS', N'Sounds/Weapons/Guns/SubMachine Guns/Kris/SMGKrisReload', N'24.06', N'0', N'1', N'371', N'38', N'0', N'0', N'0', N'0', N'1', N'4.2', N'0.43', N'331', N'5.3', N'17.95', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410655', N'0', N'0', N'410688', N'0', N'0', N'0', N'0', N'SMG_Kris', N'3300')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112966', N'SNP_FWS15_Tier2', N'21', N'$SNP_FWS15_Mk3', N'$SNP_FWS15_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/Bullistic/ARS', N'Sounds/Weapons/Guns/Sniper Rifles/Bullistic/SNPArsReload', N'120', N'0', N'1.2', N'671', N'541', N'0', N'0', N'0', N'0', N'1', N'5.4', N'0.43', N'5', N'5', N'23.25', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410846', N'0', N'0', N'0', N'0', N'SNP_Bullistic', N'5200')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112967', N'SNP_LM7_Tier2', N'21', N'$SNP_LM7_Mk4', N'$SNP_LM7_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/LM7/LM7_A', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'150', N'0', N'2.6', N'800', N'600', N'0', N'0', N'0', N'0', N'1', N'5', N'0.43', N'5', N'3', N'28.35', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410702', N'0', N'0', N'410690', N'0', N'0', N'0', N'0', N'SNP_LM7', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112968', N'SUP_AT4_Tier2', N'24', N'$SUP_AT4_Mk5', N'$SUP_AT4_Mk5_desc', N'0 0 0', N'muzzle_sup_at4_basic_01', N'assault', N'$AT4_Ammo', N'Sounds/Weapons/Guns/Support/AT4/AT4_B', N'Sounds/Weapons/Guns/Support/AT4/AT4Reload', N'1', N'0', N'1', N'175', N'1', N'7', N'0.25', N'75', N'0', N'1', N'2.67', N'0', N'60', N'3.25', N'10.3', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410659', N'0', N'0', N'410677', N'0', N'0', N'0', N'0', N'SUP_AT4', N'5600')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112970', N'SUP_Eliminator_Tier2', N'24', N'$SUP_Eliminator_Mk5', N'$SUP_Eliminator_Mk5_desc', N'0 0 0', N'muzzle_sup_at4_basic_01', N'assault', N'$AT4_Ammo', N'Sounds/Weapons/Guns/Support/Eliminator/Eliminator', N'Sounds/Weapons/Guns/Support/Eliminator/EliminatorReload', N'1', N'0', N'0.5', N'200', N'1', N'8', N'0.25', N'100', N'0', N'1', N'2.77', N'0', N'60', N'3.25', N'10.3', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410699', N'0', N'0', N'410679', N'0', N'0', N'0', N'0', N'SUP_Eliminator', N'4000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112972', N'SUP_RPG7_Tier2', N'24', N'$SUP_RPG7_Mk5', N'$SUP_RPG7_Mk5_desc', N'0 0 0', N'muzzle_sup_rpg7_basic_01', N'assault', N'$RPG7_Ammo', N'Sounds/Weapons/Guns/Support/RPG7/RPG7', N'Sounds/Weapons/Guns/Support/RPG7/RPG7Reload', N'71', N'0', N'0.5', N'371', N'71', N'6', N'0.25', N'75', N'0', N'1', N'3.75', N'0', N'60', N'3.25', N'17.3', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410661', N'0', N'0', N'410678', N'0', N'0', N'0', N'0', N'SUP_RPG7', N'4000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112979', N'SHG_SP12_Tier1', N'22', N'$SHG_SP12_Mk1', N'$SHG_SP12_Mk1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/SP12/SP12', N'Sounds/Weapons/Guns/Shotguns/SP12/SP12Reload_Bullet', N'36', N'0', N'1', N'179', N'6', N'0', N'0', N'0', N'0', N'1', N'8', N'1.2', N'21', N'6.1', N'11.61', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410650', N'0', N'0', N'410682', N'0', N'0', N'0', N'0', N'SHG_SP12', N'3300')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112980', N'SNP_AAR_Tier2', N'21', N'$SNP_AAR_Mk3', N'$SNP_AAR_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/Archangel/R13', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'115', N'0', N'2.4', N'750', N'375', N'0', N'0', N'0', N'0', N'1', N'4.3', N'0.43', N'6', N'2', N'21.25', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'32', N'2024', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410667', N'0', N'0', N'0', N'0', N'SNP_ArchAngel', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'112998', N'ASR_TRX4_Tier2', N'20', N'$ASR_TRX4_Mk3', N'$ASR_TRX4_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/Tar30/Tar39', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'24', N'0', N'1', N'471', N'56', N'0', N'0', N'0', N'0', N'1', N'5', N'0.43', N'381', N'4.9', N'15.3', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'410696', N'410853', N'0', N'0', N'410854', N'0', N'0', N'0', N'0', N'ASR_Tar30', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113003', N'HG_MicroUzi_Tier2', N'25', N'$HG_MicroUzi_Mk3', N'$HG_MicroUzi_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/MicroUzi/MicroUzi', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/UziReload', N'23', N'0', N'1', N'341', N'35', N'0', N'0', N'0', N'0', N'1', N'3.01', N'1.2', N'241', N'4.5', N'17.31', N'0', N'asr_grenade', N'001', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410851', N'0', N'0', N'410845', N'0', N'0', N'0', N'0', N'HG_MicroUzi', N'2500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113004', N'PDW_HSE_Tier2', N'25', N'$SMG_HSE_Mk3', N'$SMG_HSE_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/Uzi', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/UziReload', N'24', N'0', N'1', N'431', N'34', N'0', N'0', N'0', N'0', N'1', N'3.01', N'1.2', N'351', N'5.1', N'14.61', N'0', N'asr_grenade', N'001', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410654', N'0', N'0', N'410687', N'0', N'0', N'0', N'0', N'SMG_HSE', N'2500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113012', N'ASR_AK12_Tier1', N'20', N'$ASR_AK12_Mk2', N'$ASR_AK12_Mk2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/AK12/AK12', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'371', N'65', N'0', N'0', N'0', N'0', N'1', N'3.7', N'0.43', N'341', N'6', N'15', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'30', N'0', N'0', N'0', N'0', N'410696', N'410219', N'0', N'0', N'410664', N'0', N'0', N'0', N'0', N'ASR_AK12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113013', N'ASR_AK12_Tier2', N'20', N'$ASR_AK12_Mk3', N'$ASR_AK12_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/AK12/AK12', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'371', N'65', N'0', N'0', N'0', N'0', N'1', N'3.7', N'0.43', N'341', N'6', N'16', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'30', N'2000', N'0', N'0', N'0', N'410696', N'410219', N'0', N'0', N'410664', N'0', N'0', N'0', N'0', N'ASR_AK12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113014', N'ASR_AK12_Tier2', N'20', N'$ASR_AK12_Mk4', N'$ASR_AK12_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/AK12/AK12', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'371', N'65', N'0', N'0', N'0', N'0', N'1', N'3.7', N'0.43', N'341', N'6', N'18', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'30', N'2000', N'0', N'0', N'0', N'410696', N'410219', N'0', N'0', N'410664', N'0', N'0', N'0', N'0', N'ASR_AK12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113015', N'ASR_AK12_Tier2', N'20', N'$ASR_AK12_Mk5', N'$ASR_AK12_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/AK12/AK12', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'371', N'65', N'0', N'0', N'0', N'0', N'1', N'3.7', N'0.43', N'341', N'6', N'18', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'30', N'2000', N'0', N'0', N'0', N'410696', N'410219', N'0', N'0', N'410664', N'0', N'0', N'0', N'0', N'ASR_AK12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113016', N'ASR_AK12_Mk6_1', N'20', N'$ASR_AK12_Mk6_1', N'$ASR_AK12_Mk6_1_Desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/AK12/AK12', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'371', N'65', N'0', N'0', N'0', N'0', N'1', N'3.7', N'0.43', N'341', N'6.6', N'18.75', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'30', N'0', N'0', N'0', N'0', N'410696', N'410219', N'0', N'0', N'410664', N'0', N'0', N'0', N'0', N'ASR_AK12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113017', N'ASR_WDX_Tier1', N'20', N'$ASR_WDX_Mk2', N'$ASR_WDX_Mk2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HB_01_Wolf/HB_Wolf', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'321', N'65', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'220', N'4', N'14.67', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'410696', N'410641', N'0', N'0', N'410670', N'0', N'0', N'0', N'0', N'ASR_HB_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113018', N'ASR_WDX_Tier2', N'20', N'$ASR_WDX_Mk3', N'$ASR_WDX_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HB_01_Wolf/HB_Wolf', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'321', N'65', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'220', N'4.5', N'14.62', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'2002', N'0', N'0', N'0', N'410696', N'410641', N'0', N'0', N'410670', N'0', N'0', N'0', N'0', N'ASR_HB_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113019', N'ASR_WDX_Tier2', N'20', N'$ASR_WDX_Mk4', N'$ASR_WDX_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HB_01_Wolf/HB_Wolf', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'321', N'65', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'220', N'4', N'14.53', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'2002', N'0', N'0', N'0', N'410696', N'410641', N'0', N'0', N'410670', N'0', N'0', N'0', N'0', N'ASR_HB_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113020', N'ASR_WDX_Tier2', N'20', N'$ASR_WDX_Mk5', N'$ASR_WDX_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HB_01_Wolf/HB_Wolf', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'321', N'65', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'220', N'4', N'14.48', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'2002', N'0', N'0', N'0', N'410696', N'410641', N'0', N'0', N'410670', N'0', N'0', N'0', N'0', N'ASR_HB_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113021', N'ASR_WDX_Mk6_1', N'20', N'$ASR_WDX_Mk6_1', N'$ASR_WDX_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HB_01_Wolf/HB_Wolf', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'321', N'65', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'220', N'3.9', N'14.3', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'410696', N'410641', N'0', N'0', N'410670', N'0', N'0', N'0', N'0', N'ASR_HB_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113022', N'ASR_HB_Tier1', N'20', N'$ASR_HB_Mk2', N'$ASR_HB_Mk2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'25', N'0', N'1', N'341', N'80', N'0', N'0', N'0', N'0', N'1', N'3.9', N'0.43', N'381', N'5', N'16', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410640', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_HB', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113023', N'ASR_HB_Tier2', N'20', N'$ASR_HB_Mk3', N'$ASR_HB_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'25', N'0', N'1', N'341', N'80', N'0', N'0', N'0', N'0', N'1', N'3.9', N'0.43', N'381', N'5', N'16', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'2003', N'0', N'0', N'0', N'410696', N'410640', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_HB', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113024', N'ASR_HB_Tier2', N'20', N'$ASR_HB_Mk4', N'$ASR_HB_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'25', N'0', N'1', N'341', N'80', N'0', N'0', N'0', N'0', N'1', N'3.9', N'0.43', N'381', N'5', N'16', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'2003', N'0', N'0', N'0', N'410696', N'410640', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_HB', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113025', N'ASR_HB_Tier2', N'20', N'$ASR_HB_Mk5', N'$ASR_HB_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'25', N'0', N'1', N'341', N'80', N'0', N'0', N'0', N'0', N'1', N'3.9', N'0.43', N'381', N'5', N'16', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'2003', N'0', N'0', N'0', N'410696', N'410640', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_HB', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113026', N'ASR_HB_Mk6_1', N'20', N'$ASR_HB_Mk6_1', N'$ASR_HB_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'25', N'0', N'1', N'341', N'80', N'0', N'0', N'0', N'0', N'1', N'3.9', N'0.43', N'381', N'5', N'16', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410640', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_HB', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113027', N'HG_9B_Tier1', N'25', N'$HG_9B_Mk2', N'$HG_9B_Mk2_desc', N'0 0 0', N'muzzle_asr_basic_01', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_9mm/9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_9mm/HG_9mm_Reload', N'1.05', N'0', N'1', N'500', N'1', N'0', N'0', N'0', N'0', N'1', N'3.1', N'1.2', N'360', N'9.9', N'11.01', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410644', N'0', N'0', N'410215', N'0', N'0', N'0', N'0', N'HG_9mm', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113028', N'HG_9B_Tier2', N'25', N'$HG_9B_Mk3', N'$HG_9B_Mk3_desc', N'0 0 0', N'muzzle_asr_basic_01', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_9mm/9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_9mm/HG_9mm_Reload', N'23', N'0', N'1', N'351', N'32', N'0', N'0', N'0', N'0', N'1', N'3.1', N'1.2', N'221', N'3.15', N'15.96', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'3', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'2006', N'0', N'0', N'0', N'0', N'410644', N'0', N'0', N'410215', N'0', N'0', N'0', N'0', N'HG_9mm', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113029', N'HG_9B_Mk6_1', N'25', N'$HG_9B_Mk6_1', N'$HG_9B_Mk6_1_Desc', N'0 0 0', N'muzzle_asr_basic_01', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_9mm/9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_9mm/HG_9mm_Reload', N'23', N'0', N'1', N'351', N'32', N'0', N'0', N'0', N'0', N'1', N'3.1', N'1.2', N'221', N'3.65', N'15.91', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410644', N'0', N'0', N'410215', N'0', N'0', N'0', N'0', N'HG_9mm', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113030', N'HG_45APC_Tier2', N'25', N'$HG_45c_Mk3', N'$HG_45c_Mk3_desc', N'0 0 0', N'muzzle_asr_basic_01', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/45c', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/HG_45c_Reload', N'22', N'0', N'1', N'351', N'27', N'0', N'0', N'0', N'0', N'1', N'3.1', N'1.2', N'330', N'4.5', N'11.15', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'3', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'48', N'0', N'0', N'0', N'0', N'0', N'410645', N'0', N'0', N'410662', N'0', N'0', N'0', N'0', N'HG_45c', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113031', N'HG_45APC_Tier2', N'25', N'$HG_45c_Mk4', N'$HG_45c_Mk4_desc', N'0 0 0', N'muzzle_asr_basic_01', N'assault', N'$Pistol_Ammo45', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/45c', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/HG_45c_Reload', N'22', N'0', N'1', N'351', N'27', N'0', N'0', N'0', N'0', N'1', N'3.1', N'1.2', N'330', N'4.2', N'11.05', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'7', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'48', N'0', N'0', N'0', N'0', N'0', N'410645', N'0', N'0', N'410662', N'0', N'0', N'0', N'0', N'HG_45c', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113032', N'HG_45APC_Mk6_1', N'25', N'$HG_45c_Mk6_1', N'$HG_45c_Mk6_1_desc', N'0 0 0', N'muzzle_asr_basic_01', N'assault', N'$Pistol_Ammo45', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/45c', N'Sounds/Weapons/Guns/Pistols/Pistol_45c/HG_45c_Reload', N'22', N'0', N'1', N'351', N'27', N'0', N'0', N'0', N'0', N'1', N'3.1', N'1.2', N'330', N'4.6', N'11.95', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'48', N'0', N'0', N'0', N'0', N'0', N'410645', N'0', N'0', N'410662', N'0', N'0', N'0', N'0', N'HG_45c', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113033', N'PDW_HB2_Tier2', N'25', N'$SMG_HB_02_Mk3', N'$SMG_HB_02_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/HB_02_SMG/HB_02', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/UziReload', N'23', N'0', N'1', N'341', N'31', N'0', N'0', N'0', N'0', N'1', N'3.01', N'1.2', N'330', N'4', N'11.01', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410653', N'0', N'0', N'410686', N'0', N'0', N'0', N'0', N'SMG_HB_02', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113034', N'PDW_HSE_Tier2', N'25', N'$SMG_HSE_Mk4', N'$SMG_HSE_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/Uzi', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/UziReload', N'24', N'0', N'0.5', N'451', N'35', N'0', N'0', N'0', N'0', N'1', N'3.01', N'1.2', N'351', N'4', N'14.56', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410654', N'0', N'0', N'410687', N'0', N'0', N'0', N'0', N'SMG_HSE', N'2500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113035', N'PDW_HSE_Tier2', N'25', N'$SMG_HSE_Mk5', N'$SMG_HSE_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/Uzi', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/UziReload', N'24', N'0', N'0.5', N'451', N'35', N'0', N'0', N'0', N'0', N'1', N'3.01', N'1.2', N'351', N'4', N'14.51', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410654', N'0', N'0', N'410687', N'0', N'0', N'0', N'0', N'SMG_HSE', N'2500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113036', N'SMG_Bizon_Tier1', N'26', N'$SMG_Bizon_Mk2', N'$SMG_Bizon_Mk2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/BIZON_mod', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/SMGBizonReload', N'23', N'0', N'1', N'350', N'35', N'0', N'0', N'0', N'0', N'1', N'4.6', N'0.43', N'391', N'5.7', N'15.2', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410651', N'0', N'0', N'410683', N'0', N'0', N'0', N'0', N'SMG_Bizon', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113037', N'SMG_Bizon_Tier2', N'26', N'$SMG_Bizon_Mk3', N'$SMG_Bizon_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/BIZON_mod', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/SMGBizonReload', N'23', N'0', N'1', N'350', N'35', N'0', N'0', N'0', N'0', N'1', N'4.6', N'0.43', N'391', N'5.65', N'15.15', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'2013', N'0', N'0', N'0', N'410696', N'410651', N'0', N'0', N'410683', N'0', N'0', N'0', N'0', N'SMG_Bizon', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113038', N'SMG_Bizon_Tier2', N'26', N'$SMG_Bizon_Mk4', N'$SMG_Bizon_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/BIZON_mod', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/SMGBizonReload', N'23', N'0', N'1', N'350', N'35', N'0', N'0', N'0', N'0', N'1', N'4.6', N'0.43', N'391', N'5.65', N'15.1', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'2013', N'0', N'0', N'0', N'410696', N'410651', N'0', N'0', N'410683', N'0', N'0', N'0', N'0', N'SMG_Bizon', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113039', N'SMG_Bizon_Tier2', N'26', N'$SMG_Bizon_Mk5', N'$SMG_Bizon_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/BIZON_mod', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/SMGBizonReload', N'23', N'0', N'1', N'350', N'35', N'0', N'0', N'0', N'0', N'1', N'4.6', N'0.43', N'391', N'5.65', N'15.05', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'2013', N'0', N'0', N'0', N'410696', N'410651', N'0', N'0', N'410683', N'0', N'0', N'0', N'0', N'SMG_Bizon', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113040', N'SMG_Bizon_Mk6_1', N'26', N'$SMG_Bizon_Mk6_1', N'$SMG_Bizon_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/BIZON_mod', N'Sounds/Weapons/Guns/SubMachine Guns/Bizon/SMGBizonReload', N'23', N'0', N'1', N'350', N'35', N'0', N'0', N'0', N'0', N'1', N'4.6', N'0.43', N'391', N'5', N'15.95', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410651', N'0', N'0', N'410683', N'0', N'0', N'0', N'0', N'SMG_Bizon', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113041', N'SMG_EVO_Tier1', N'26', N'$SMG_EVO_Mk2', N'$SMG_EVO_Mk2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/EVO_mod', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/SMBEvoReload', N'23', N'0', N'1', N'410', N'35', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'321', N'3', N'14', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'77', N'0', N'0', N'0', N'0', N'410696', N'410652', N'0', N'0', N'410684', N'0', N'0', N'0', N'0', N'SMG_EVO', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113042', N'SMG_EVO_Tier2', N'26', N'$SMG_EVO_Mk3', N'$SMG_EVO_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/EVO_mod', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/SMBEvoReload', N'24', N'0', N'1', N'410', N'35', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'321', N'3', N'14', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'77', N'0', N'0', N'0', N'0', N'410696', N'410652', N'0', N'0', N'410684', N'0', N'0', N'0', N'0', N'SMG_EVO', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113043', N'SMG_EVO_Tier2', N'26', N'$SMG_EVO_Mk4', N'$SMG_EVO_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/EVO_mod', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/SMBEvoReload', N'24', N'0', N'1', N'410', N'35', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'321', N'3', N'14', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'77', N'0', N'0', N'0', N'0', N'410696', N'410652', N'0', N'0', N'410684', N'0', N'0', N'0', N'0', N'SMG_EVO', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113044', N'SMG_EVO_Tier2', N'26', N'$SMG_EVO_Mk5', N'$SMG_EVO_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/EVO_mod', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/SMBEvoReload', N'24', N'0', N'1', N'410', N'35', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'321', N'3', N'14', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'77', N'0', N'0', N'0', N'0', N'410696', N'410652', N'0', N'0', N'410684', N'0', N'0', N'0', N'0', N'SMG_EVO', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113045', N'SMG_EVO_Mk6_1', N'26', N'$SMG_EVO_Mk6_1', N'$SMG_EVO_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/EVO_mod', N'Sounds/Weapons/Guns/SubMachine Guns/EVO/SMBEvoReload', N'24', N'0', N'1', N'410', N'35', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'321', N'3', N'14', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'77', N'0', N'0', N'0', N'0', N'410696', N'410652', N'0', N'0', N'410684', N'0', N'0', N'0', N'0', N'SMG_EVO', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113046', N'SNP_AAR_Tier2', N'21', N'$SNP_AAR_Mk4', N'$SNP_AAR_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/Archangel/R13', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'115', N'0', N'1.1', N'650', N'650', N'0', N'0', N'0', N'0', N'1', N'3', N'0.43', N'6', N'2', N'21.25', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'32', N'2024', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410667', N'0', N'0', N'0', N'0', N'SNP_ArchAngel', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113047', N'SNP_ARS_Tier2', N'21', N'$SNP_ARS_Mk5', N'$SNP_ARS_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/ARS', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'90', N'0', N'1.1', N'500', N'451', N'0', N'0', N'0', N'0', N'1', N'4.76', N'0.43', N'8', N'3', N'26.25', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'2018', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410689', N'0', N'0', N'0', N'0', N'SNP_ARS', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113048', N'SNP_ARS_Mk6_1', N'21', N'$SNP_ARS_Mk6_1', N'$SNP_ARS_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/ARS', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'90', N'0', N'1.1', N'500', N'451', N'0', N'0', N'0', N'0', N'1', N'4.76', N'0.43', N'8', N'3.4', N'26.95', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410689', N'0', N'0', N'0', N'0', N'SNP_ARS', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113049', N'SHG_SP12_Tier1', N'22', N'$SHG_SP12_Mk2', N'$SHG_SP12_Mk2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/SP12/SP12', N'Sounds/Weapons/Guns/Shotguns/SP12/SP12Reload_Bullet', N'36.1', N'0', N'1', N'179', N'6', N'0', N'0', N'0', N'0', N'1', N'2.74', N'1.2', N'21', N'8.2', N'15.51', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410650', N'0', N'0', N'410682', N'0', N'0', N'0', N'0', N'SHG_SP12', N'3000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113050', N'SHG_SP12_Mk6_1', N'22', N'$SHG_SP12_Mk6_1', N'$SHG_SP12_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/SP12/SP12', N'Sounds/Weapons/Guns/Shotguns/SP12/SP12Reload_Bullet', N'36', N'0', N'1', N'179', N'6', N'0', N'0', N'0', N'0', N'1', N'2.74', N'1.2', N'6', N'8.9', N'17.41', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410650', N'0', N'0', N'410682', N'0', N'0', N'0', N'0', N'SHG_SP12', N'3000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113051', N'SHG_M1216_Tier2', N'22', N'$SHG_M1216_Mk4', N'$SHG_M1216_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/M1216/M1216', N'Sounds/Weapons/Guns/Shotguns/M1216/M1216_Reload', N'30', N'0', N'1', N'251', N'1', N'0', N'0', N'0', N'0', N'1', N'3', N'1.2', N'23', N'7.7', N'16.56', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410848', N'0', N'0', N'410844', N'0', N'0', N'0', N'0', N'SHG_M1216', N'3000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113052', N'SHG_M1216_Tier2', N'22', N'$SHG_M1216_Mk5', N'$SHG_M1216_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/M1216/M1216', N'Sounds/Weapons/Guns/Shotguns/M1216/M1216_Reload', N'30', N'0', N'1', N'251', N'8', N'0', N'0', N'0', N'0', N'1', N'3', N'1.2', N'23', N'5.9', N'16.51', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410848', N'0', N'0', N'410844', N'0', N'0', N'0', N'0', N'SHG_M1216', N'3000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113054', N'ASR_ACX10_Tier2', N'20', N'$ASR_ACX10_Mk3', N'$ASR_ACX10_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'27', N'0', N'1', N'421', N'41', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'300', N'3.75', N'14.35', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'2007', N'0', N'0', N'0', N'410696', N'410639', N'0', N'0', N'410669', N'0', N'0', N'0', N'0', N'ASR_AR_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113055', N'ASR_ACX10_Tier2', N'20', N'$ASR_ACX10_Mk4', N'$ASR_ACX10_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'27', N'0', N'1', N'421', N'1', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'300', N'3.85', N'14.3', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'2007', N'0', N'0', N'0', N'410696', N'410639', N'0', N'0', N'410669', N'0', N'0', N'0', N'0', N'ASR_AR_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113056', N'ASR_ACX10_Tier2', N'20', N'$ASR_ACX10_Mk5', N'$ASR_ACX10_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'27', N'0', N'1', N'421', N'1', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'300', N'3.65', N'14.25', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'2007', N'0', N'0', N'0', N'410696', N'410639', N'0', N'0', N'410669', N'0', N'0', N'0', N'0', N'ASR_AR_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113057', N'ASR_ACX10_Mk6_1', N'20', N'$ASR_ACX10_Mk6_1', N'$ASR_ACX10_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/Honeybadger', N'Sounds/Weapons/Guns/Assault Rifles/HoneyBadger/HoneyBadgerReload', N'27', N'0', N'1', N'421', N'41', N'0', N'0', N'0', N'0', N'1', N'3.8', N'0.43', N'300', N'3.15', N'14.15', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410639', N'0', N'0', N'410669', N'0', N'0', N'0', N'0', N'ASR_AR_01', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113058', N'ASR_ASh12_Tier2', N'20', N'$ASR_ASh12_Mk4', N'$ASR_ASh12_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/Ash12/Ash12', N'Sounds/Weapons/Guns/Assault Rifles/Ash12/Ash12_Reload', N'21', N'0', N'1', N'500', N'1', N'0', N'0', N'0', N'0', N'1', N'2.93', N'0.43', N'291', N'4.65', N'16.15', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410847', N'0', N'0', N'410841', N'0', N'0', N'0', N'0', N'ASR_ASh12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113059', N'ASR_ASh12_Tier2', N'20', N'$ASR_ASh12_Mk5', N'$ASR_ASh12_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/Ash12/Ash12', N'Sounds/Weapons/Guns/Assault Rifles/Ash12/Ash12_Reload', N'21', N'0', N'1', N'321', N'35', N'0', N'0', N'0', N'0', N'1', N'2.93', N'0.43', N'291', N'4.65', N'16.15', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410847', N'0', N'0', N'410841', N'0', N'0', N'0', N'0', N'ASR_ASh12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113060', N'ASR_ASh12_Mk6_1', N'20', N'$ASR_ASh12_Mk6_1', N'$ASR_ASh12_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/Ash12/Ash12', N'Sounds/Weapons/Guns/Assault Rifles/Ash12/Ash12_Reload', N'20.5', N'0', N'1', N'321', N'35', N'0', N'0', N'0', N'0', N'1', N'2.93', N'0.43', N'291', N'4.65', N'16.05', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410847', N'0', N'0', N'410841', N'0', N'0', N'0', N'0', N'ASR_ASh12', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113062', N'ASR_MXZ5_Tier2', N'20', N'$ASR_MXZ5_Mk3', N'$ASR_MXZ5_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/MXZ5/MXZ5', N'Sounds/Weapons/Guns/Assault Rifles/MXZ5/MXZ5Reload', N'26', N'0', N'1', N'451', N'1', N'0', N'0', N'0', N'0', N'1', N'3.4', N'0.43', N'321', N'5.3', N'17.3', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'2004', N'0', N'0', N'0', N'410696', N'410642', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_MXZ5', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113063', N'ASR_MXZ5_Tier2', N'20', N'$ASR_MXZ5_Mk4', N'$ASR_MXZ5_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/MXZ5/MXZ5', N'Sounds/Weapons/Guns/Assault Rifles/MXZ5/MXZ5Reload', N'24', N'0', N'1', N'451', N'1', N'0', N'0', N'0', N'0', N'1', N'3.4', N'0.43', N'321', N'4.1', N'14.2', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'2004', N'0', N'0', N'0', N'410696', N'410642', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_MXZ5', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113064', N'ASR_MXZ5_Tier2', N'20', N'$ASR_MXZ5_Mk5', N'$ASR_MXZ5_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/MXZ5/MXZ5', N'Sounds/Weapons/Guns/Assault Rifles/MXZ5/MXZ5Reload', N'24', N'0', N'1', N'451', N'1', N'0', N'0', N'0', N'0', N'1', N'3.4', N'0.43', N'321', N'4.1', N'14.1', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'2004', N'0', N'0', N'0', N'410696', N'410642', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_MXZ5', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113065', N'ASR_MXZ5_Mk6_1', N'20', N'$ASR_MXZ5_Mk6_1', N'$ASR_MXZ5_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/MXZ5/MXZ5', N'Sounds/Weapons/Guns/Assault Rifles/MXZ5/MXZ5Reload', N'24', N'0', N'1', N'451', N'56', N'0', N'0', N'0', N'0', N'1', N'3.4', N'0.43', N'321', N'4.1', N'14', N'0', N'asr_grenade', N'111', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410642', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_MXZ5', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113067', N'ASR_THOR5_Tier2', N'20', N'$ASR_THOR-5_Mk3', N'$ASR_THOR-5_Mk3_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/SHRAM', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'25', N'0', N'1', N'341', N'53', N'0', N'0', N'0', N'0', N'1', N'4.95', N'0.43', N'270', N'13.7', N'18.3', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410643', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_SHRAM', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113068', N'ASR_THOR5_Tier2', N'20', N'$ASR_THOR-5_Mk4', N'$ASR_THOR-5_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/SHRAM', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'351', N'66', N'0', N'0', N'0', N'0', N'1', N'4.95', N'0.43', N'391', N'4.65', N'15.25', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410643', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_SHRAM', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113069', N'ASR_THOR5_Tier2', N'20', N'$ASR_THOR-5_Mk5', N'$ASR_THOR-5_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/SHRAM', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'351', N'66', N'0', N'0', N'0', N'0', N'1', N'4.95', N'0.43', N'391', N'4.65', N'15.2', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410643', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_SHRAM', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113070', N'ASR_THOR5_Mk6_1', N'20', N'$ASR_THOR-5_Mk6_1', N'$ASR_THOR-5_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/SHRAM', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'451', N'67', N'0', N'0', N'0', N'0', N'1', N'4.95', N'0.43', N'391', N'4.65', N'15.1', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410643', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_SHRAM', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113071', N'ASR_TRX4_Tier2', N'20', N'$ASR_TRX4_Mk4', N'$ASR_TRX4_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/Tar30/Tar39', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'24', N'0', N'1', N'471', N'56', N'0', N'0', N'0', N'0', N'1', N'5', N'0.43', N'381', N'4.9', N'15.25', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'410696', N'410853', N'0', N'0', N'410854', N'0', N'0', N'0', N'0', N'ASR_Tar30', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113072', N'ASR_TRX4_Tier2', N'20', N'$ASR_TRX4_Mk5', N'$ASR_TRX4_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/Tar30/Tar39', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'24', N'0', N'1', N'471', N'56', N'0', N'0', N'0', N'0', N'1', N'5', N'0.43', N'381', N'4.9', N'15.2', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'410696', N'410853', N'0', N'0', N'410854', N'0', N'0', N'0', N'0', N'ASR_Tar30', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113073', N'ASR_TRX4_Mk6_1', N'20', N'$ASR_TRX4_Mk6_1', N'$ASR_TRX4_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/Tar30/Tar39', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'24', N'0', N'1', N'471', N'56', N'0', N'0', N'0', N'0', N'1', N'5', N'0.43', N'381', N'4.9', N'15.15', N'0', N'asr_grenade', N'111', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'410696', N'410853', N'0', N'0', N'410854', N'0', N'0', N'0', N'0', N'ASR_Tar30', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113074', N'SNP_LM7_Tier2', N'21', N'$SNP_LM7_Mk5', N'$SNP_LM7_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/LM7/LM7_A', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'150', N'0', N'1.3', N'800', N'600', N'0', N'0', N'0', N'0', N'1', N'3', N'0.43', N'4', N'3', N'28.35', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410702', N'0', N'0', N'410690', N'0', N'0', N'0', N'0', N'SNP_LM7', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113075', N'SNP_FWS15_Tier2', N'21', N'$SNP_FWS15_Mk4', N'$SNP_FWS15_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/Bullistic/ARS', N'Sounds/Weapons/Guns/Sniper Rifles/Bullistic/SNPArsReload', N'120', N'0', N'1.2', N'671', N'1', N'0', N'0', N'0', N'0', N'1', N'5.4', N'0.43', N'5', N'3', N'28.25', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410846', N'0', N'0', N'0', N'0', N'SNP_Bullistic', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113076', N'SNP_FWS15_Tier2', N'21', N'$SNP_FWS15_Mk5', N'$SNP_FWS15_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/Bullistic/ARS', N'Sounds/Weapons/Guns/Sniper Rifles/Bullistic/SNPArsReload', N'120', N'0', N'1.2', N'671', N'1', N'0', N'0', N'0', N'0', N'1', N'5.4', N'0.43', N'5', N'3', N'23.25', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410846', N'0', N'0', N'0', N'0', N'SNP_Bullistic', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113077', N'SNP_FWS15_Mk6_1', N'21', N'$SNP_FWS15_Mk6_1', N'$SNP_FWS15_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/Bullistic/ARS', N'Sounds/Weapons/Guns/Sniper Rifles/Bullistic/SNPArsReload', N'120', N'0', N'1.2', N'671', N'541', N'0', N'0', N'0', N'0', N'1', N'5.4', N'0.43', N'5', N'3', N'23.95', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410846', N'0', N'0', N'0', N'0', N'SNP_Bullistic', N'6000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113078', N'SHG_Saiga_Tier1', N'22', N'$SHG_Saiga_Mk2', N'$SHG_Saiga_Mk2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/Saiga/Saiga', N'Sounds/Weapons/Guns/Shotguns/Saiga/SaigaReload', N'25.5', N'0', N'1', N'221', N'1', N'0', N'0', N'0', N'0', N'1', N'3.92', N'1.2', N'40', N'5.4', N'18.51', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'88', N'0', N'0', N'0', N'0', N'410697', N'410649', N'0', N'0', N'410680', N'0', N'0', N'0', N'0', N'SHG_Saiga', N'3000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113079', N'LMG_NG100_Tier2', N'23', N'$LMG_NG100_Mk4', N'$LMG_NG100_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/LargeMachine Guns/NG100/NG100', N'Sounds/Weapons/Guns/LargeMachine Guns/NG100/LMGNG100Reload_v2', N'36', N'0', N'1', N'600', N'45', N'0', N'0', N'0', N'0', N'1', N'6.24', N'0', N'341', N'5', N'17', N'0', N'asr_grenade', N'001', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410648', N'0', N'0', N'410676', N'0', N'0', N'0', N'0', N'LMG_NG100', N'4000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113080', N'LMG_NG100_Tier2', N'23', N'$LMG_NG100_Mk5', N'$LMG_NG100_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/LargeMachine Guns/NG100/NG100', N'Sounds/Weapons/Guns/LargeMachine Guns/NG100/LMGNG100Reload_v2', N'36', N'0', N'1', N'451', N'45', N'0', N'0', N'0', N'0', N'1', N'6.24', N'0', N'341', N'5', N'17', N'0', N'asr_grenade', N'001', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410648', N'0', N'0', N'410676', N'0', N'0', N'0', N'0', N'LMG_NG100', N'4000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113081', N'LMG_NG100_Mk6_1', N'23', N'$LMG_NG100_Mk6_1', N'$LMG_NG100_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/LargeMachine Guns/NG100/NG100', N'Sounds/Weapons/Guns/LargeMachine Guns/NG100/LMGNG100Reload_v2', N'36', N'0', N'1', N'600', N'45', N'0', N'0', N'0', N'0', N'1', N'6.24', N'0', N'341', N'5', N'17', N'0', N'asr_grenade', N'001', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410648', N'0', N'0', N'410676', N'0', N'0', N'0', N'0', N'LMG_NG100', N'4000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113082', N'HG_AP50_Tier2', N'25', N'$HG_AP50_Mk5', N'$HG_AP50_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo50', N'Sounds/Weapons/Guns/Pistols/AP50/AP50', N'', N'45', N'0', N'1', N'341', N'11', N'0', N'0', N'0', N'0', N'1', N'2.7', N'1.2', N'89', N'5.5', N'26.15', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410850', N'0', N'0', N'410843', N'0', N'0', N'0', N'0', N'HG_AP50', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113083', N'HG_AP50_Mk6_1', N'25', N'$HG_AP50_Mk6_1', N'$HG_AP50_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo50', N'Sounds/Weapons/Guns/Pistols/AP50/AP50', N'', N'45', N'0', N'1', N'341', N'11', N'0', N'0', N'0', N'0', N'1', N'2.7', N'1.2', N'89', N'5.5', N'26.85', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410850', N'0', N'0', N'410843', N'0', N'0', N'0', N'0', N'HG_AP50', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113084', N'HG_RV50_Tier2', N'25', N'$HG_RV50_Mk5', N'$HG_RV50_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo50', N'Sounds/Weapons/Guns/Pistols/RV50c/RV50c', N'Sounds/Weapons/Guns/Pistols/RV50c/RV50c_Reload', N'37', N'0', N'1', N'371', N'25', N'0', N'0', N'0', N'0', N'1', N'4.14', N'1.2', N'90', N'5.5', N'29.15', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410646', N'0', N'0', N'410673', N'0', N'0', N'0', N'0', N'HG_RV50c', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113085', N'HG_RV50_Mk6_1', N'25', N'$HG_RV50_Mk6_1', N'$HG_RV50_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo50', N'Sounds/Weapons/Guns/Pistols/RV50c/RV50c', N'Sounds/Weapons/Guns/Pistols/RV50c/RV50c_Reload', N'37', N'0', N'1', N'371', N'25', N'0', N'0', N'0', N'0', N'1', N'4.14', N'1.2', N'90', N'5.5', N'29.85', N'0', N'asr_grenade', N'100', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410646', N'0', N'0', N'410673', N'0', N'0', N'0', N'0', N'HG_RV50c', N'1800')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113086', N'HG_MicroUzi_Tier2', N'25', N'$HG_MicroUzi_Mk4', N'$HG_MicroUzi_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/MicroUzi/MicroUzi', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/UziReload', N'23', N'0', N'1', N'341', N'1', N'0', N'0', N'0', N'0', N'1', N'3.01', N'1.2', N'300', N'4.6', N'15.21', N'0', N'asr_grenade', N'001', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410851', N'0', N'0', N'410845', N'0', N'0', N'0', N'0', N'HG_MicroUzi', N'2500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113087', N'HG_MicroUzi_Tier2', N'25', N'$HG_MicroUzi_Mk5', N'$HG_MicroUzi_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/Pistols/MicroUzi/MicroUzi', N'Sounds/Weapons/Guns/SubMachine Guns/HSE/UziReload', N'23', N'0', N'1', N'341', N'35', N'0', N'0', N'0', N'0', N'1', N'3.01', N'1.2', N'300', N'4.6', N'15.21', N'0', N'asr_grenade', N'001', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'2', N'2', N'2', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410851', N'0', N'0', N'410845', N'0', N'0', N'0', N'0', N'HG_MicroUzi', N'2500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113088', N'SMG_Kris_Tier2', N'26', N'$SMG_Kris_Mk5', N'$SMG_Kris_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo45', N'Sounds/Weapons/Guns/SubMachine Guns/Kris/KRIS', N'Sounds/Weapons/Guns/SubMachine Guns/Kris/SMGKrisReload', N'24', N'0', N'1', N'371', N'1', N'0', N'0', N'0', N'0', N'1', N'4.2', N'0.43', N'331', N'10.9', N'11.9', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410655', N'0', N'0', N'410688', N'0', N'0', N'0', N'0', N'SMG_Kris', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113089', N'SMG_Kris_Mk6_1', N'26', N'$SMG_Kris_Mk6_1', N'$SMG_Kris_Mk6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo45', N'Sounds/Weapons/Guns/SubMachine Guns/Kris/KRIS', N'Sounds/Weapons/Guns/SubMachine Guns/Kris/SMGKrisReload', N'24', N'0', N'1', N'371', N'1', N'0', N'0', N'0', N'0', N'1', N'4.2', N'0.43', N'331', N'10.9', N'11.75', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410655', N'0', N'0', N'410688', N'0', N'0', N'0', N'0', N'SMG_Kris', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113090', N'SMG_CF10_Tier2', N'26', N'$SMG_CF-10_Mk4', N'$SMG_CF-10_Mk4_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/ChangFeng/ChangFeng', N'Sounds/Weapons/Guns/SubMachine Guns/ChangFeng/SMGBizonReload', N'21', N'0', N'1', N'341', N'31', N'0', N'0', N'0', N'0', N'1', N'3.51', N'0.43', N'251', N'4.1', N'12.1', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410849', N'0', N'0', N'410840', N'0', N'0', N'0', N'0', N'SMG_ChangFeng', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113091', N'SMG_CF10_Tier2', N'26', N'$SMG_CF-10_Mk5', N'$SMG_CF-10_Mk5_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Pistol_Ammo9mm', N'Sounds/Weapons/Guns/SubMachine Guns/ChangFeng/ChangFeng', N'Sounds/Weapons/Guns/SubMachine Guns/ChangFeng/SMGBizonReload', N'21', N'0', N'1', N'341', N'31', N'0', N'0', N'0', N'0', N'1', N'3.51', N'0.43', N'251', N'4.1', N'14.05', N'0', N'asr_grenade', N'101', N'30', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'410696', N'410849', N'0', N'0', N'410840', N'0', N'0', N'0', N'0', N'SMG_ChangFeng', N'3500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113092', N'MEL_Knife_MK1', N'29', N'$MEL_Knife_MK1', N'$MEL_Knife_MK1_desc', N'0 0 0', N'', N'melee', N'melee_sharp', N'Sounds/Weapons/Melee/Knife/Melee_Knife', N'', N'20', N'1', N'10', N'300', N'1', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'100', N'6', N'12', N'0', N'asr_grenade', N'001', N'15', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'MEL_Tactical_Knife', N'500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113093', N'MEL_Knife_MK2', N'29', N'$MEL_Knife_MK2', N'$MEL_Knife_MK2_desc', N'0 0 0', N'', N'melee', N'melee_sharp', N'Sounds/Weapons/Melee/Knife/Melee_Knife', N'', N'20', N'1', N'10', N'300', N'1', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'100', N'6', N'12', N'0', N'asr_grenade', N'001', N'15', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'5', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'MEL_Tactical_Knife', N'500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113094', N'MEL_Knife_MK3', N'29', N'$MEL_Knife_MK3', N'$MEL_Knife_MK3_desc', N'0 0 0', N'', N'melee', N'melee_sharp', N'Sounds/Weapons/Melee/Knife/Melee_Knife', N'', N'20', N'1', N'10', N'300', N'1', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'100', N'6', N'12', N'0', N'asr_grenade', N'001', N'15', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'10', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'MEL_Tactical_Knife', N'500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113095', N'MEL_Knife_MK4', N'29', N'$MEL_Knife_MK4', N'$MEL_Knife_MK4_desc', N'0 0 0', N'', N'melee', N'melee_sharp', N'Sounds/Weapons/Melee/Knife/Melee_Knife', N'', N'20', N'1', N'10', N'300', N'1', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'100', N'6', N'12', N'0', N'asr_grenade', N'001', N'15', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'15', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'MEL_Tactical_Knife', N'500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113096', N'MEL_Knife_MK5', N'29', N'$MEL_Knife_MK5', N'$MEL_Knife_MK5_desc', N'0 0 0', N'', N'melee', N'melee_sharp', N'Sounds/Weapons/Melee/Knife/Melee_Knife', N'', N'20', N'1', N'10', N'300', N'1', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'100', N'6', N'12', N'0', N'asr_grenade', N'001', N'15', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'MEL_Tactical_Knife', N'500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113098', N'ASR_THOR5_Mk6_2', N'20', N'$ASR_THOR-5_Mk6_2', N'$ASR_THOR-5_Mk6_2_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$ASR_Ammo', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/SHRAM', N'Sounds/Weapons/Guns/Assault Rifles/SHRAM/ShramReload', N'26', N'0', N'1', N'321', N'65', N'0', N'0', N'0', N'0', N'1', N'3.95', N'0.43', N'241', N'4.8', N'15.05', N'0', N'asr_grenade', N'101', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'30', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'1', N'1', N'1', N'1', N'35', N'0', N'0', N'0', N'0', N'410696', N'410643', N'0', N'0', N'410691', N'0', N'0', N'0', N'0', N'ASR_SHRAM', N'4500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113101', N'MEL_Knife_MK6', N'29', N'$MEL_Knife_MK6', N'$MEL_Knife_MK6_desc', N'0 0 0', N'', N'melee', N'melee_sharp', N'Sounds/Weapons/Melee/Knife/Melee_Knife', N'', N'20', N'1', N'10', N'300', N'1', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'100', N'6', N'12', N'0', N'asr_grenade', N'001', N'15', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'MEL_Tactical_Knife', N'500')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113102', N'SUP_RPG7_Tier2', N'24', N'$Alien_Missile', N'Projectile attack for aliens', N'0 0 0', N'muzzle_asr', N'assault', N'$Alien_projectile', N'', N'', N'1', N'0', N'0.01', N'15', N'1', N'6', N'0.25', N'10', N'0', N'1', N'3.75', N'0', N'60', N'7', N'15', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'SUP_RPG7', N'1')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113105', N'SHG_M1216_MK6_1', N'22', N'$SHG_M1216_MK6_1', N'$SHG_M1216_MK6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Shotgun_Ammo', N'Sounds/Weapons/Guns/Shotguns/M1216/M1216', N'Sounds/Weapons/Guns/Shotguns/M1216/M1216_Reload', N'30', N'0', N'1', N'251', N'8', N'0', N'0', N'0', N'0', N'1', N'3', N'3.2', N'23', N'7.4', N'16.61', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410848', N'0', N'0', N'410844', N'0', N'0', N'0', N'0', N'SHG_M1216', N'3000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113106', N'SUP_RPG7_MK6_1', N'24', N'$SUP_RPG7_Mk6_1', N'$SUP_RPG7_Mk6_1_desc', N'0 0 0', N'muzzle_sup_rpg7_basic_01', N'assault', N'$RPG7_Ammo', N'Sounds/Weapons/Guns/Support/RPG7/RPG7', N'Sounds/Weapons/Guns/Support/RPG7/RPG7Reload', N'1', N'0', N'0.5', N'190', N'1', N'6', N'0.25', N'75', N'0', N'1', N'3.75', N'0', N'60', N'3.25', N'10.3', N'0', N'asr_grenade', N'100', N'50', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'410661', N'0', N'0', N'410678', N'0', N'0', N'0', N'0', N'SUP_RPG7', N'4000')
GO

INSERT INTO [dbo].[Items_Weapons] ([ItemID], [FNAME], [Category], [Name], [Description], [MuzzleOffset], [MuzzleParticle], [Animation], [BulletID], [Sound_Shot], [Sound_Reload], [Damage], [isImmediate], [Mass], [Speed], [DamageDecay], [Area], [Delay], [Timeout], [NumClips], [Clipsize], [ReloadTime], [ActiveReloadTick], [RateOfFire], [Spread], [Recoil], [NumGrenades], [GrenadeName], [Firemode], [DetectionRadius], [ScopeType], [ScopeZoom], [Price1], [Price7], [Price30], [PriceP], [IsNew], [LevelRequired], [GPrice1], [GPrice7], [GPrice30], [GPriceP], [ShotsFired], [ShotsHits], [KillsCQ], [KillsDM], [KillsSB], [IsUpgradeable], [IsFPS], [FPSSpec0], [FPSSpec1], [FPSSpec2], [FPSSpec3], [FPSSpec4], [FPSSpec5], [FPSSpec6], [FPSSpec7], [FPSSpec8], [FPSAttach0], [FPSAttach1], [FPSAttach2], [FPSAttach3], [FPSAttach4], [FPSAttach5], [FPSAttach6], [FPSAttach7], [FPSAttach8], [AnimPrefix], [Weight]) VALUES (N'113107', N'SNP_AAR_MK6_1', N'21', N'$SNP_AAR_MK6_1', N'$SNP_AAR_MK6_1_desc', N'0 0 0', N'muzzle_asr', N'assault', N'$Sniper_Ammo', N'Sounds/Weapons/Guns/Sniper Rifles/Archangel/R13', N'Sounds/Weapons/Guns/Sniper Rifles/ARS/SNPArsReload', N'115', N'0', N'1.1', N'750', N'400', N'0', N'0', N'0', N'0', N'1', N'3', N'0.43', N'5', N'2', N'21.25', N'0', N'asr_grenade', N'100', N'35', N'default', N'0', N'0', N'0', N'0', N'0', N'1', N'20', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'0', N'1', N'1', N'0', N'1', N'0', N'0', N'32', N'0', N'0', N'0', N'0', N'0', N'410703', N'0', N'0', N'410667', N'0', N'0', N'0', N'0', N'SNP_ArchAngel', N'6000')
GO

-- ----------------------------
-- Table structure for Leaderboard
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Leaderboard]') AND type IN ('U'))
	DROP TABLE [dbo].[Leaderboard]
GO

CREATE TABLE [dbo].[Leaderboard] (
  [Pos] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NOT NULL,
  [gamertag] nvarchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [Rank] int  NOT NULL,
  [HonorPoints] int  NOT NULL,
  [Wins] int  NOT NULL,
  [Losses] int  NOT NULL,
  [Kills] int  NOT NULL,
  [Deaths] int  NOT NULL,
  [ShotsFired] int  NOT NULL,
  [ShotsHit] int  NOT NULL,
  [TimePlayed] int  NOT NULL,
  [HavePremium] int  NOT NULL
)
GO

ALTER TABLE [dbo].[Leaderboard] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Leaderboard
-- ----------------------------
SET IDENTITY_INSERT [dbo].[Leaderboard] ON
GO

SET IDENTITY_INSERT [dbo].[Leaderboard] OFF
GO


-- ----------------------------
-- Table structure for Leaderboard1
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Leaderboard1]') AND type IN ('U'))
	DROP TABLE [dbo].[Leaderboard1]
GO

CREATE TABLE [dbo].[Leaderboard1] (
  [Pos] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NOT NULL,
  [gamertag] nvarchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [Rank] int  NOT NULL,
  [HonorPoints] int  NOT NULL,
  [Wins] int  NOT NULL,
  [Losses] int  NOT NULL,
  [Kills] int  NOT NULL,
  [Deaths] int  NOT NULL,
  [ShotsFired] int  NOT NULL,
  [ShotsHit] int  NOT NULL,
  [TimePlayed] int  NOT NULL,
  [HavePremium] int  NOT NULL
)
GO

ALTER TABLE [dbo].[Leaderboard1] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Leaderboard1
-- ----------------------------
SET IDENTITY_INSERT [dbo].[Leaderboard1] ON
GO

SET IDENTITY_INSERT [dbo].[Leaderboard1] OFF
GO


-- ----------------------------
-- Table structure for Leaderboard30
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Leaderboard30]') AND type IN ('U'))
	DROP TABLE [dbo].[Leaderboard30]
GO

CREATE TABLE [dbo].[Leaderboard30] (
  [Pos] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NOT NULL,
  [gamertag] nvarchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [Rank] int  NOT NULL,
  [HonorPoints] int  NOT NULL,
  [Wins] int  NOT NULL,
  [Losses] int  NOT NULL,
  [Kills] int  NOT NULL,
  [Deaths] int  NOT NULL,
  [ShotsFired] int  NOT NULL,
  [ShotsHit] int  NOT NULL,
  [TimePlayed] int  NOT NULL,
  [HavePremium] int  NOT NULL
)
GO

ALTER TABLE [dbo].[Leaderboard30] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Leaderboard30
-- ----------------------------
SET IDENTITY_INSERT [dbo].[Leaderboard30] ON
GO

SET IDENTITY_INSERT [dbo].[Leaderboard30] OFF
GO


-- ----------------------------
-- Table structure for Leaderboard7
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Leaderboard7]') AND type IN ('U'))
	DROP TABLE [dbo].[Leaderboard7]
GO

CREATE TABLE [dbo].[Leaderboard7] (
  [Pos] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NOT NULL,
  [gamertag] nvarchar(32) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [Rank] int  NOT NULL,
  [HonorPoints] int  NOT NULL,
  [Wins] int  NOT NULL,
  [Losses] int  NOT NULL,
  [Kills] int  NOT NULL,
  [Deaths] int  NOT NULL,
  [ShotsFired] int  NOT NULL,
  [ShotsHit] int  NOT NULL,
  [TimePlayed] int  NOT NULL,
  [HavePremium] int  NOT NULL
)
GO

ALTER TABLE [dbo].[Leaderboard7] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Leaderboard7
-- ----------------------------
SET IDENTITY_INSERT [dbo].[Leaderboard7] ON
GO

SET IDENTITY_INSERT [dbo].[Leaderboard7] OFF
GO


-- ----------------------------
-- Table structure for Logins
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[Logins]') AND type IN ('U'))
	DROP TABLE [dbo].[Logins]
GO

CREATE TABLE [dbo].[Logins] (
  [LoginID] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int DEFAULT 0 NOT NULL,
  [LoginTime] datetime DEFAULT ('19731201') NOT NULL,
  [IP] varchar(16) COLLATE Cyrillic_General_CI_AS DEFAULT '1.1.1.1' NOT NULL,
  [LoginSource] int DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[Logins] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of Logins
-- ----------------------------
SET IDENTITY_INSERT [dbo].[Logins] ON
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'1', N'1000000', N'2026-06-15 01:36:32.617', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'2', N'1000000', N'2026-06-15 02:59:20.233', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'3', N'1000000', N'2026-06-15 02:59:20.303', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'4', N'1000000', N'2026-06-15 03:01:39.457', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'5', N'1000000', N'2026-06-15 03:01:39.463', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'6', N'1000000', N'2026-06-15 03:05:04.570', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'7', N'1000000', N'2026-06-15 03:05:04.623', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'8', N'1000000', N'2026-06-15 03:19:18.867', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'9', N'1000000', N'2026-06-15 03:24:30.550', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'10', N'1000000', N'2026-06-15 03:25:34.797', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'11', N'1000000', N'2026-06-15 03:29:32.387', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'12', N'1000000', N'2026-06-15 03:35:36.350', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'13', N'1000000', N'2026-06-15 03:48:00.587', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'14', N'1000000', N'2026-06-16 17:32:50.437', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'15', N'1000000', N'2026-06-16 18:19:10.287', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'16', N'1000000', N'2026-06-16 18:21:02.740', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'17', N'1000000', N'2026-06-16 18:24:19.217', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'18', N'1000000', N'2026-06-16 18:25:46.607', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'19', N'1000000', N'2026-06-16 18:26:04.790', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'20', N'1000000', N'2026-06-16 18:30:40.950', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'21', N'1000000', N'2026-06-16 18:31:49.247', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'22', N'1000000', N'2026-06-16 18:32:20.723', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'23', N'1000000', N'2026-06-16 18:33:53.483', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'24', N'1000000', N'2026-06-16 18:35:17.657', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'25', N'1000000', N'2026-06-16 18:37:05.307', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'26', N'1000000', N'2026-06-16 18:39:43.563', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'27', N'1000000', N'2026-06-16 18:40:09.700', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'28', N'1000000', N'2026-06-16 18:41:20.207', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'29', N'1000000', N'2026-06-16 18:54:15.210', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'30', N'1000000', N'2026-06-16 18:56:11.003', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'31', N'1000000', N'2026-06-16 18:59:27.143', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'32', N'1000000', N'2026-06-16 19:00:20.273', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'33', N'1000000', N'2026-06-16 19:01:40.620', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'34', N'1000000', N'2026-06-16 19:02:05.560', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'35', N'1000000', N'2026-06-16 19:03:38.570', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'36', N'1000000', N'2026-06-16 19:04:11.403', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'37', N'1000000', N'2026-06-16 19:07:10.310', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'38', N'1000000', N'2026-06-16 19:20:00.250', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'39', N'1000000', N'2026-06-16 19:21:46.590', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'40', N'1000000', N'2026-06-16 19:23:38.763', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'41', N'1000000', N'2026-06-16 19:24:05.070', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'42', N'1000000', N'2026-06-16 19:30:23.037', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'43', N'1000000', N'2026-06-16 19:34:10.900', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'44', N'1000000', N'2026-06-16 19:36:22.020', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'45', N'1000000', N'2026-06-16 19:41:05.077', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'46', N'1000000', N'2026-06-16 19:44:01.290', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'47', N'1000000', N'2026-06-16 19:44:45.217', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'48', N'1000000', N'2026-06-16 19:45:16.690', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'49', N'1000000', N'2026-06-16 20:01:03.767', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'50', N'1000000', N'2026-06-17 01:10:11.397', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'51', N'1000000', N'2026-06-17 01:13:12.817', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'52', N'1000000', N'2026-06-17 01:16:23.557', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'53', N'1000000', N'2026-06-17 01:18:07.833', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'54', N'1000000', N'2026-06-17 01:23:30.100', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'55', N'1000000', N'2026-06-17 01:26:50.850', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'56', N'1000000', N'2026-06-17 01:28:09.410', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'57', N'1000000', N'2026-06-17 01:30:05.837', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'58', N'1000000', N'2026-06-17 01:49:38.957', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'59', N'1000000', N'2026-06-17 01:50:54.043', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'60', N'1000000', N'2026-06-17 01:57:00.630', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'61', N'1000000', N'2026-06-17 02:04:04.993', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'62', N'1000000', N'2026-06-17 02:05:11.240', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'63', N'1000000', N'2026-06-17 02:06:15.563', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'64', N'1000000', N'2026-06-17 02:07:18.130', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'65', N'1000000', N'2026-06-17 02:13:29.487', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'66', N'1000000', N'2026-06-17 02:15:08.590', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'67', N'1000000', N'2026-06-17 02:19:23.050', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'68', N'1000000', N'2026-06-17 02:19:40.040', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'69', N'1000000', N'2026-06-17 02:21:26.770', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'70', N'1000000', N'2026-06-17 02:21:47.353', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'71', N'1000000', N'2026-06-17 02:22:16.880', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'72', N'1000000', N'2026-06-17 02:25:12.190', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'73', N'1000000', N'2026-06-17 02:26:33.360', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'74', N'1000000', N'2026-06-17 02:29:20.390', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'75', N'1000000', N'2026-06-17 02:31:39.660', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'76', N'1000000', N'2026-06-17 02:34:33.713', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'77', N'1000000', N'2026-06-17 02:35:03.913', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'78', N'1000000', N'2026-06-17 02:36:05.427', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'79', N'1000000', N'2026-06-17 02:36:48.233', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'80', N'1000000', N'2026-06-17 02:46:31.450', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'81', N'1000000', N'2026-06-17 02:46:53.273', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'82', N'1000000', N'2026-06-17 02:48:27.770', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'83', N'1000000', N'2026-06-17 02:49:08.717', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'84', N'1000000', N'2026-06-17 02:49:56.330', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'85', N'1000000', N'2026-06-17 02:52:54.150', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'86', N'1000000', N'2026-06-17 02:54:04.230', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'87', N'1000000', N'2026-06-17 02:56:19.403', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'88', N'1000000', N'2026-06-17 02:58:51.530', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'89', N'1000000', N'2026-06-17 03:03:12.473', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'90', N'1000000', N'2026-06-17 03:09:47.370', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'91', N'1000000', N'2026-06-17 03:10:36.590', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'92', N'1000000', N'2026-06-18 01:02:42.660', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'93', N'1000000', N'2026-06-18 01:24:22.833', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'94', N'1000000', N'2026-06-18 01:34:12.360', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'95', N'1000000', N'2026-06-18 01:41:32.867', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'96', N'1000000', N'2026-06-18 01:42:46.793', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'97', N'1000000', N'2026-06-18 01:43:11.377', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'98', N'1000000', N'2026-06-18 02:00:24.203', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'99', N'1000000', N'2026-06-18 02:22:19.867', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'100', N'1000000', N'2026-06-18 02:47:53.540', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'101', N'1000000', N'2026-06-18 02:54:34.263', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'102', N'1000000', N'2026-06-18 03:02:54.360', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'103', N'1000000', N'2026-06-18 03:07:47.840', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'104', N'1000000', N'2026-06-18 03:08:21.213', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'105', N'1000000', N'2026-06-18 03:23:06.270', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'106', N'1000000', N'2026-06-18 03:24:19.867', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'107', N'1000000', N'2026-06-18 03:24:41.197', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'108', N'1000000', N'2026-06-18 03:29:00.397', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'109', N'1000000', N'2026-06-18 03:36:06.633', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'110', N'1000000', N'2026-06-18 03:37:42.293', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'111', N'1000000', N'2026-06-18 03:40:32.127', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'112', N'1000000', N'2026-06-18 03:41:32.570', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'113', N'1000000', N'2026-06-18 03:42:21.170', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'114', N'1000000', N'2026-06-18 03:43:04.620', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'115', N'1000000', N'2026-06-18 03:43:26.950', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'116', N'1000000', N'2026-06-18 03:43:41.783', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'117', N'1000000', N'2026-06-18 03:45:28.110', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'118', N'1000000', N'2026-06-18 03:48:40.097', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'119', N'1000000', N'2026-06-18 03:50:20.317', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'120', N'1000000', N'2026-06-18 03:51:43.653', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'121', N'1000000', N'2026-06-18 03:52:32.433', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'122', N'1000000', N'2026-06-18 04:08:43.363', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'123', N'1000000', N'2026-06-18 19:06:30.283', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'124', N'1000000', N'2026-06-19 22:21:57.180', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'125', N'1000000', N'2026-06-19 22:22:35.257', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'126', N'1000000', N'2026-06-19 22:46:47.560', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'127', N'1000000', N'2026-06-19 22:54:34.437', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'128', N'1000000', N'2026-06-19 22:55:40.963', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'129', N'1000000', N'2026-06-19 23:17:37.703', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'130', N'1000000', N'2026-06-19 23:22:40.807', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'131', N'1000000', N'2026-06-19 23:25:28.190', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'132', N'1000000', N'2026-06-19 23:28:12.137', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'133', N'1000000', N'2026-06-19 23:36:54.573', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'134', N'1000000', N'2026-06-19 23:37:28.727', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'135', N'1000000', N'2026-06-19 23:39:29.340', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'136', N'1000000', N'2026-06-19 23:39:47.810', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'137', N'1000000', N'2026-06-19 23:46:15.943', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'138', N'1000000', N'2026-06-19 23:47:08.993', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'139', N'1000000', N'2026-06-19 23:49:12.887', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'140', N'1000000', N'2026-06-19 23:50:39.637', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'141', N'1000000', N'2026-06-19 23:51:24.797', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'142', N'1000000', N'2026-06-20 00:04:53.410', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'143', N'1000000', N'2026-06-20 00:16:05.187', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'144', N'1000000', N'2026-06-20 00:18:07.880', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'145', N'1000000', N'2026-06-20 01:10:09.350', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'146', N'1000000', N'2026-06-20 01:16:24.440', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'147', N'1000000', N'2026-06-20 01:19:13.047', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'148', N'1000000', N'2026-06-20 01:22:26.803', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'149', N'1000000', N'2026-06-20 01:23:37.863', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'150', N'1000000', N'2026-06-20 01:26:39.520', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'151', N'1000000', N'2026-06-20 01:27:37.920', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'152', N'1000000', N'2026-06-20 01:29:48.227', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'153', N'1000000', N'2026-06-20 01:43:53.137', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'154', N'1000000', N'2026-06-20 01:44:23.207', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'155', N'1000000', N'2026-06-20 01:50:09.920', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'156', N'1000000', N'2026-06-20 02:16:51.473', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'157', N'1000000', N'2026-06-20 03:01:35.043', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'158', N'1000000', N'2026-06-20 03:07:56.670', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'159', N'1000000', N'2026-06-20 03:20:33.537', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'160', N'1000000', N'2026-06-20 03:34:07.257', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'161', N'1000000', N'2026-06-20 03:38:57.920', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'162', N'1000000', N'2026-06-20 03:46:34.200', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'163', N'1000000', N'2026-06-20 03:55:05.040', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'164', N'1000000', N'2026-06-20 03:57:09.660', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'165', N'1000000', N'2026-06-20 04:04:56.300', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'166', N'1000000', N'2026-06-20 04:09:28.090', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'167', N'1000000', N'2026-06-20 04:14:25.600', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'168', N'1000000', N'2026-06-20 10:31:43.817', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'169', N'1000000', N'2026-06-20 11:30:04.103', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'170', N'1000000', N'2026-06-20 11:42:30.063', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'171', N'1000000', N'2026-06-20 11:42:44.990', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'172', N'1000000', N'2026-06-20 11:46:58.360', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'173', N'1000000', N'2026-06-20 12:07:52.800', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'174', N'1000000', N'2026-06-20 12:09:43.130', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'175', N'1000000', N'2026-06-20 12:19:08.467', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'176', N'1000000', N'2026-06-20 12:22:27.633', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'177', N'1000000', N'2026-06-20 12:24:30.310', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'178', N'1000000', N'2026-06-20 12:25:11.210', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'179', N'1000000', N'2026-06-20 14:34:15.820', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'180', N'1000000', N'2026-06-20 15:33:44.097', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'181', N'1000000', N'2026-06-24 22:14:41.263', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'182', N'1000000', N'2026-06-24 22:47:18.027', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'183', N'1000000', N'2026-06-24 22:55:33.490', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'184', N'1000000', N'2026-06-24 23:16:49.940', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'185', N'1000000', N'2026-06-25 00:28:06.633', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'186', N'1000000', N'2026-06-25 01:08:57.453', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'187', N'1000000', N'2026-06-25 01:10:18.300', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'188', N'1000000', N'2026-06-25 01:11:37.213', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'189', N'1000000', N'2026-06-25 01:15:35.800', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'190', N'1000000', N'2026-06-25 01:22:06.970', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'191', N'1000000', N'2026-06-25 01:34:39.427', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'192', N'1000000', N'2026-06-25 01:37:32.193', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'193', N'1000000', N'2026-06-26 23:22:50.910', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'194', N'1000000', N'2026-06-27 00:02:46.917', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'195', N'1000000', N'2026-06-27 00:06:48.863', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'196', N'1000000', N'2026-06-27 00:10:26.843', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'197', N'1000000', N'2026-06-27 00:11:13.737', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'198', N'1000000', N'2026-06-27 00:57:44.947', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'199', N'1000000', N'2026-06-27 01:11:59.860', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'200', N'1000000', N'2026-06-27 01:13:59.393', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'201', N'1000000', N'2026-06-27 01:18:56.840', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'202', N'1000000', N'2026-06-27 01:28:02.953', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'203', N'1000000', N'2026-06-27 01:29:21.373', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'204', N'1000000', N'2026-06-27 01:31:41.833', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'205', N'1000000', N'2026-06-27 01:41:07.120', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'206', N'1000000', N'2026-06-27 01:42:20.420', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'207', N'1000000', N'2026-06-27 01:44:05.513', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'208', N'1000000', N'2026-06-27 01:45:27.150', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'209', N'1000000', N'2026-06-27 01:49:36.973', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'210', N'1000000', N'2026-06-27 21:23:18.487', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'211', N'1000000', N'2026-06-27 21:28:16.513', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'212', N'1000000', N'2026-06-27 21:30:12.387', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'213', N'1000000', N'2026-06-27 21:30:43.850', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'214', N'1000000', N'2026-07-01 20:59:05.463', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'215', N'1000000', N'2026-07-01 22:10:56.780', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'216', N'1000000', N'2026-07-01 22:28:22.667', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'217', N'1000000', N'2026-07-01 22:32:25.577', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'218', N'1000000', N'2026-07-01 22:38:59.960', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'219', N'1000000', N'2026-07-01 22:49:28.727', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'220', N'1000000', N'2026-07-01 23:07:49.510', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'221', N'1000000', N'2026-07-01 23:14:00.110', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'222', N'1000000', N'2026-07-01 23:38:31.190', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'223', N'1000000', N'2026-07-02 23:11:09.463', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'224', N'1000000', N'2026-07-02 23:36:20.243', N'26.163.92.76', N'0')
GO

INSERT INTO [dbo].[Logins] ([LoginID], [CustomerID], [LoginTime], [IP], [LoginSource]) VALUES (N'225', N'1000000', N'2026-07-02 23:40:11.700', N'26.163.92.76', N'0')
GO

SET IDENTITY_INSERT [dbo].[Logins] OFF
GO


-- ----------------------------
-- Table structure for LoginSessions
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[LoginSessions]') AND type IN ('U'))
	DROP TABLE [dbo].[LoginSessions]
GO

CREATE TABLE [dbo].[LoginSessions] (
  [CustomerID] int  NOT NULL,
  [SessionKey] varchar(50) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [SessionID] int DEFAULT 0 NOT NULL,
  [LoginIP] varchar(16) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [TimeLogged] datetime  NOT NULL,
  [TimeUpdated] datetime  NOT NULL,
  [GameSessionID] bigint DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[LoginSessions] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of LoginSessions
-- ----------------------------
INSERT INTO [dbo].[LoginSessions] ([CustomerID], [SessionKey], [SessionID], [LoginIP], [TimeLogged], [TimeUpdated], [GameSessionID]) VALUES (N'1000000', N'9EDE88EA-9599-467D-9355-CFB00BECFB74', N'1030736035', N'26.163.92.76', N'2026-07-02 23:40:11.697', N'2026-07-02 23:40:11.707', N'0')
GO


-- ----------------------------
-- Table structure for MasterServerInfo
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[MasterServerInfo]') AND type IN ('U'))
	DROP TABLE [dbo].[MasterServerInfo]
GO

CREATE TABLE [dbo].[MasterServerInfo] (
  [ServerID] int DEFAULT 0 NOT NULL,
  [LastUpdated] datetime DEFAULT ('19700101') NOT NULL,
  [CreateGameKey] int DEFAULT 0 NOT NULL,
  [IP] varchar(64) COLLATE Cyrillic_General_CI_AS DEFAULT '0.0.0.0' NOT NULL
)
GO

ALTER TABLE [dbo].[MasterServerInfo] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of MasterServerInfo
-- ----------------------------

-- ----------------------------
-- Table structure for SecurityLog
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[SecurityLog]') AND type IN ('U'))
	DROP TABLE [dbo].[SecurityLog]
GO

CREATE TABLE [dbo].[SecurityLog] (
  [ID] int  IDENTITY(1,1) NOT NULL,
  [EventID] int  NOT NULL,
  [Date] datetime  NOT NULL,
  [IP] varchar(64) COLLATE Cyrillic_General_CI_AS DEFAULT '0.0.0.0' NOT NULL,
  [CustomerID] int DEFAULT 0 NOT NULL,
  [EventData] varchar(256) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL
)
GO

ALTER TABLE [dbo].[SecurityLog] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of SecurityLog
-- ----------------------------
SET IDENTITY_INSERT [dbo].[SecurityLog] ON
GO

SET IDENTITY_INSERT [dbo].[SecurityLog] OFF
GO


-- ----------------------------
-- Table structure for ServerNotesData
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[ServerNotesData]') AND type IN ('U'))
	DROP TABLE [dbo].[ServerNotesData]
GO

CREATE TABLE [dbo].[ServerNotesData] (
  [NoteID] int  IDENTITY(1,1) NOT NULL,
  [GameServerId] int  NOT NULL,
  [GamePos] varchar(128) COLLATE Cyrillic_General_CI_AS  NULL,
  [CreateUtcDate] datetime  NULL,
  [ExpireUtcDate] datetime  NULL,
  [CustomerID] int  NULL,
  [CharID] int  NULL,
  [TextFrom] nvarchar(128) COLLATE Cyrillic_General_CI_AS  NULL,
  [TextSubj] nvarchar(2048) COLLATE Cyrillic_General_CI_AS  NULL
)
GO

ALTER TABLE [dbo].[ServerNotesData] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of ServerNotesData
-- ----------------------------
SET IDENTITY_INSERT [dbo].[ServerNotesData] ON
GO

SET IDENTITY_INSERT [dbo].[ServerNotesData] OFF
GO


-- ----------------------------
-- Table structure for UsersChars
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[UsersChars]') AND type IN ('U'))
	DROP TABLE [dbo].[UsersChars]
GO

CREATE TABLE [dbo].[UsersChars] (
  [CharID] int  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NOT NULL,
  [Hardcore] int  NOT NULL,
  [Gamertag] nvarchar(64) COLLATE Cyrillic_General_CI_AS  NOT NULL,
  [HeroItemID] int DEFAULT 20754 NOT NULL,
  [HeadIdx] int  NOT NULL,
  [BodyIdx] int  NOT NULL,
  [LegsIdx] int  NOT NULL,
  [HairIdx] int DEFAULT 0 NOT NULL,
  [FeetIdx] int DEFAULT 0 NOT NULL,
  [Alive] int DEFAULT 3 NOT NULL,
  [DeathUtcTime] datetime DEFAULT '1/1/1973' NOT NULL,
  [XP] int DEFAULT 0 NOT NULL,
  [TimePlayed] int DEFAULT 0 NOT NULL,
  [GameMapId] int DEFAULT 0 NOT NULL,
  [GameServerId] bigint DEFAULT 0 NOT NULL,
  [GamePos] varchar(128) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [GameFlags] int DEFAULT 0 NOT NULL,
  [Health] float(53) DEFAULT 100 NOT NULL,
  [Food] float(53) DEFAULT 0 NOT NULL,
  [Water] float(53) DEFAULT 0 NOT NULL,
  [Toxic] float(53) DEFAULT 0 NOT NULL,
  [Reputation] int DEFAULT 0 NOT NULL,
  [BackpackID] int DEFAULT 21712 NOT NULL,
  [BackpackSize] int DEFAULT 16 NOT NULL,
  [Attachment1] varchar(256) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [Attachment2] varchar(256) COLLATE Cyrillic_General_CI_AS DEFAULT '' NOT NULL,
  [Stat00] int DEFAULT 0 NOT NULL,
  [Stat01] int DEFAULT 0 NOT NULL,
  [Stat02] int DEFAULT 0 NOT NULL,
  [Stat03] int DEFAULT 0 NOT NULL,
  [Stat04] int DEFAULT 0 NOT NULL,
  [Stat05] int DEFAULT 0 NOT NULL,
  [LastUpdateDate] datetime  NULL,
  [CreateDate] datetime  NULL,
  [ClanID] int DEFAULT 0 NOT NULL,
  [ClanRank] int DEFAULT 99 NOT NULL,
  [ClanContributedXP] int DEFAULT 0 NOT NULL,
  [ClanContributedGP] int DEFAULT 0 NOT NULL
)
GO

ALTER TABLE [dbo].[UsersChars] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of UsersChars
-- ----------------------------
SET IDENTITY_INSERT [dbo].[UsersChars] ON
GO

INSERT INTO [dbo].[UsersChars] ([CharID], [CustomerID], [Hardcore], [Gamertag], [HeroItemID], [HeadIdx], [BodyIdx], [LegsIdx], [HairIdx], [FeetIdx], [Alive], [DeathUtcTime], [XP], [TimePlayed], [GameMapId], [GameServerId], [GamePos], [GameFlags], [Health], [Food], [Water], [Toxic], [Reputation], [BackpackID], [BackpackSize], [Attachment1], [Attachment2], [Stat00], [Stat01], [Stat02], [Stat03], [Stat04], [Stat05], [LastUpdateDate], [CreateDate], [ClanID], [ClanRank], [ClanContributedXP], [ClanContributedGP]) VALUES (N'3', N'1000000', N'0', N'ES3N1N', N'20754', N'0', N'0', N'0', N'0', N'0', N'1', N'1973-01-01 00:00:00.000', N'0', N'60', N'2', N'1', N'7178.074 162.786 2267.629 37', N'0', N'100', N'1', N'1', N'0', N'0', N'21712', N'16', N'0 0 0 0 0 0 0 0', N'0 0 0 0 0 0 0 0', N'0', N'0', N'0', N'0', N'0', N'0', N'2026-06-26 23:25:39.710', N'2026-06-16 17:32:07.013', N'0', N'99', N'0', N'0')
GO

SET IDENTITY_INSERT [dbo].[UsersChars] OFF
GO


-- ----------------------------
-- Table structure for UsersData
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[UsersData]') AND type IN ('U'))
	DROP TABLE [dbo].[UsersData]
GO

CREATE TABLE [dbo].[UsersData] (
  [CustomerID] int  NOT NULL,
  [IsDeveloper] int DEFAULT 0 NOT NULL,
  [AccountType] int DEFAULT 99 NOT NULL,
  [AccountStatus] int DEFAULT 100 NOT NULL,
  [GamePoints] int DEFAULT 0 NOT NULL,
  [GameDollars] int DEFAULT 0 NOT NULL,
  [dateregistered] datetime DEFAULT '1/1/1973' NOT NULL,
  [lastjoineddate] datetime DEFAULT '1/1/1973' NOT NULL,
  [lastgamedate] datetime DEFAULT '1/1/1973' NOT NULL,
  [ClanID] int DEFAULT 0 NOT NULL,
  [ClanRank] int DEFAULT 99 NOT NULL,
  [GameServerId] bigint  NULL,
  [CharsCreated] int DEFAULT 0 NOT NULL,
  [TimePlayed] int DEFAULT 0 NOT NULL,
  [DateActiveUntil] datetime DEFAULT '2030-1-1' NOT NULL,
  [BanTime] datetime  NULL,
  [BanReason] nvarchar(512) COLLATE Cyrillic_General_CI_AS  NULL,
  [BanCount] int DEFAULT 0 NOT NULL,
  [BanExpireDate] datetime  NULL
)
GO

ALTER TABLE [dbo].[UsersData] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of UsersData
-- ----------------------------
INSERT INTO [dbo].[UsersData] ([CustomerID], [IsDeveloper], [AccountType], [AccountStatus], [GamePoints], [GameDollars], [dateregistered], [lastjoineddate], [lastgamedate], [ClanID], [ClanRank], [GameServerId], [CharsCreated], [TimePlayed], [DateActiveUntil], [BanTime], [BanReason], [BanCount], [BanExpireDate]) VALUES (N'1000000', N'126', N'0', N'100', N'3910', N'0', N'2026-06-15 00:01:07.320', N'2026-06-26 23:23:30.790', N'2026-06-26 23:25:39.710', N'0', N'99', N'1', N'1', N'0', N'2030-01-01 00:00:00.000', NULL, NULL, N'0', NULL)
GO


-- ----------------------------
-- Table structure for UsersInventory
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[UsersInventory]') AND type IN ('U'))
	DROP TABLE [dbo].[UsersInventory]
GO

CREATE TABLE [dbo].[UsersInventory] (
  [InventoryID] bigint  IDENTITY(1,1) NOT NULL,
  [CustomerID] int  NOT NULL,
  [CharID] int DEFAULT 0 NOT NULL,
  [BackpackSlot] int DEFAULT 0 NOT NULL,
  [ItemID] int  NOT NULL,
  [LeasedUntil] datetime  NOT NULL,
  [Quantity] int DEFAULT 1 NOT NULL,
  [Var1] int DEFAULT -1 NOT NULL,
  [Var2] int DEFAULT -1 NOT NULL
)
GO

ALTER TABLE [dbo].[UsersInventory] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of UsersInventory
-- ----------------------------
SET IDENTITY_INSERT [dbo].[UsersInventory] ON
GO

INSERT INTO [dbo].[UsersInventory] ([InventoryID], [CustomerID], [CharID], [BackpackSlot], [ItemID], [LeasedUntil], [Quantity], [Var1], [Var2]) VALUES (N'16', N'1000000', N'0', N'0', N'101336', N'2020-01-01 00:00:00.000', N'2', N'-1', N'-1')
GO

INSERT INTO [dbo].[UsersInventory] ([InventoryID], [CustomerID], [CharID], [BackpackSlot], [ItemID], [LeasedUntil], [Quantity], [Var1], [Var2]) VALUES (N'17', N'1000000', N'0', N'0', N'20025', N'2020-01-01 00:00:00.000', N'2', N'-1', N'-1')
GO

INSERT INTO [dbo].[UsersInventory] ([InventoryID], [CustomerID], [CharID], [BackpackSlot], [ItemID], [LeasedUntil], [Quantity], [Var1], [Var2]) VALUES (N'12', N'1000000', N'3', N'1', N'101306', N'2020-01-01 00:00:00.000', N'1', N'0', N'-1')
GO

INSERT INTO [dbo].[UsersInventory] ([InventoryID], [CustomerID], [CharID], [BackpackSlot], [ItemID], [LeasedUntil], [Quantity], [Var1], [Var2]) VALUES (N'13', N'1000000', N'3', N'2', N'109505', N'2020-01-01 00:00:00.000', N'1', N'-1', N'-1')
GO

INSERT INTO [dbo].[UsersInventory] ([InventoryID], [CustomerID], [CharID], [BackpackSlot], [ItemID], [LeasedUntil], [Quantity], [Var1], [Var2]) VALUES (N'14', N'1000000', N'3', N'3', N'101296', N'2020-01-01 00:00:00.000', N'1', N'-1', N'-1')
GO

INSERT INTO [dbo].[UsersInventory] ([InventoryID], [CustomerID], [CharID], [BackpackSlot], [ItemID], [LeasedUntil], [Quantity], [Var1], [Var2]) VALUES (N'15', N'1000000', N'3', N'4', N'101289', N'2020-01-01 00:00:00.000', N'1', N'-1', N'-1')
GO

SET IDENTITY_INSERT [dbo].[UsersInventory] OFF
GO


-- ----------------------------
-- Table structure for VitalStats_V1
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[VitalStats_V1]') AND type IN ('U'))
	DROP TABLE [dbo].[VitalStats_V1]
GO

CREATE TABLE [dbo].[VitalStats_V1] (
  [Timestamp] datetime  NULL,
  [TotalSales] int  NULL,
  [TotalUsers] int  NULL,
  [DAU] int  NULL,
  [CCU] int  NULL,
  [Revenues] int  NULL
)
GO

ALTER TABLE [dbo].[VitalStats_V1] SET (LOCK_ESCALATION = TABLE)
GO


-- ----------------------------
-- Records of VitalStats_V1
-- ----------------------------

-- ----------------------------
-- procedure structure for ADMIN_BanUser
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[ADMIN_BanUser]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[ADMIN_BanUser]
GO

CREATE PROCEDURE [dbo].[ADMIN_BanUser]
	@in_CustomerID int,
	@in_BanReason nvarchar(256)
AS
BEGIN
	SET NOCOUNT ON;
	if(LEN(@in_BanReason) < 4) begin
		select 'GIVE PROPER REASON'
		return
	end

	declare @email varchar(128)
	select @email=email from dbo.Accounts where CustomerID=@in_CustomerID

	-- do not ban multiple times
	declare @AccountStatus int = 0
	declare @AccountType int = 0
	declare @BanCount int = 0
	declare @BanReason nvarchar(512)
	select
		@AccountStatus=AccountStatus,
		@accountType=AccountType,
		@BanCount=BanCount,
		@BanReason=BanReason
	from UsersData where CustomerID=@in_CustomerID
	if(@AccountStatus = 200 or @AccountStatus = 201) begin
		select 0 as ResultCode, 'already banned' as ResultMsg, @email as 'email'
		return
	end

	-- clear his login session
	update dbo.LoginSessions set SessionID=0 where CustomerID=@in_CustomerID

	-- set his all alive chars to respawned mode
	update dbo.UsersChars set Alive=2 where CustomerID=@in_CustomerID and Alive=1

	if(@BanReason is null) set @BanReason = @in_BanReason
	else                   set @BanReason = @in_BanReason + ', ' + @BanReason

	-- guest ban, permament
	if(@AccountType	= 3) begin
		set @BanReason = '[GUEST_BAN] ' + @BanReason
		set @BanCount  = 99;
	end

	-- ban
	if(@BanCount > 0)
	begin
		insert into dbo.DBG_BanLog values (@in_CustomerID, GETDATE(), 2000, @in_BanReason)

		update UsersData set
			AccountStatus=200,
			BanReason=@BanReason,
			BanTime=GETDATE(),
			BanCount=(BanCount+1),
			BanExpireDate='2030-1-1'
			where CustomerID=@in_CustomerID

		select 0 as ResultCode, 'Permanent BAN' as ResultMsg, @email as 'email', @BanReason as 'BanReason'
		return
	end
	else
	begin
		declare @BanTime int = 24

		insert into dbo.DBG_BanLog values (@in_CustomerID, GETDATE(), @BanTime, @in_BanReason)

		update UsersData set
			AccountStatus=201,
			BanReason=@BanReason,
			BanTime=GETDATE(),
			BanCount=(BanCount+1),
			BanExpireDate=DATEADD(hour, @BanTime, GETDATE())
			where CustomerID=@in_CustomerID

		select 0 as ResultCode, 'temporary ban' as ResultMsg, @email as 'email', @BanReason as 'BanReason'
		return
	end
END
GO


-- ----------------------------
-- procedure structure for ADMIN_BanWeaponHackers
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[ADMIN_BanWeaponHackers]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[ADMIN_BanWeaponHackers]
GO

CREATE PROCEDURE [dbo].[ADMIN_BanWeaponHackers]
AS
BEGIN
	SET NOCOUNT ON;
	declare @hacks TABLE
	(
		CustomerID int,
		data varchar(512)
	)

	declare @dt1 date = DATEADD(day, -1, GETDATE())

	-- select all hack attempts to table
	insert into @hacks
		select CustomerID, data from DBG_SrvLogInfo where
			ReportTime >= @dt1 and IsProcessed=0 and CheatID=5 and (data like 'id:%')

	-- clear them
	update DBG_SrvLogInfo set IsProcessed=1 where
		ReportTime >= @dt1 and IsProcessed=0 and CheatID=5

	--
	-- main ban loop
	--
	declare @CustomerID int
	declare @HackData varchar(512)

	DECLARE t_cursor CURSOR FOR
		select customerid, data from @hacks

	OPEN t_cursor
	FETCH NEXT FROM t_cursor into @CustomerID, @HackData
	while @@FETCH_STATUS = 0
	begin
		declare @AccountStatus int

		-- start banning
		select @AccountStatus=AccountStatus from dbo.UsersData where CustomerID=@CustomerID

		if(@AccountStatus = 100)
		begin
			declare @BanReason varchar(512) = ''
			set @BanReason = 'WH[' +
				convert(varchar(128), MONTH(GETDATE())) +
				'/' +
				convert(varchar(128), DAY(GETDATE())) +
				'] ' + @HackData

			print @CustomerID
			print @BanReason
			exec ADMIN_BanUser @CustomerID, @BanReason
		end

		FETCH NEXT FROM t_cursor into @CustomerID, @HackData
	end
	close t_cursor
	deallocate t_cursor

END
GO


-- ----------------------------
-- procedure structure for DB_PurgeUnusedUserData
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DB_PurgeUnusedUserData]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[DB_PurgeUnusedUserData]
GO

CREATE PROCEDURE [dbo].[DB_PurgeUnusedUserData]
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

	delete from Profile_Chars
	where not exists (select * from LoginID where LoginID.CustomerID = Profile_Chars.CustomerID)
	select @@RowCount as Deleted, 'Profile_Chars' as FromTable

	delete from ProfileData
	where not exists (select * from LoginID where LoginID.CustomerID = ProfileData.CustomerID)
	select @@RowCount as Deleted, 'ProfileData' as FromTable

	delete from Stats
	where not exists (select * from LoginID where LoginID.CustomerID = Stats.CustomerID)
	select @@RowCount as Deleted, 'Stats' as FromTable

	delete from Logins
	where not exists (select * from LoginID where LoginID.CustomerID = Logins.CustomerID)
	select @@RowCount as Deleted, 'Logins' as FromTable

	delete from Inventory
	where not exists (select * from LoginID where LoginID.CustomerID = Inventory.CustomerID)
	select @@RowCount as Deleted, 'Inventory' as FromTable

	delete from Inventory_FPS
	where not exists (select * from LoginID where LoginID.CustomerID = Inventory_FPS.CustomerID)
	select @@RowCount as Deleted, 'Inventory_FPS' as FromTable

	--delete from SteamUserIDMap
	--where not exists (select * from LoginID where LoginID.CustomerID = SteamUserIDMap.CustomerID)
	--select @@RowCount as Deleted, 'SteamUserIDMap' as FromTable

	--delete from GamersfirstUserIDMap
	--where not exists (select * from LoginID where LoginID.CustomerID = GamersfirstUserIDMap.CustomerID)
	--select @@RowCount as Deleted, 'GamersfirstUserIDMap' as FromTable

	-- purge inventory
	declare @InvCleanDate datetime = DATEADD(day, -30, GETDATE())
	delete from Inventory where LeasedUntil<@InvCleanDate
	delete from Inventory_FPS where LeasedUntil<@InvCleanDate

END
GO


-- ----------------------------
-- procedure structure for DBG_RegisterIISCall
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[DBG_RegisterIISCall]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[DBG_RegisterIISCall]
GO

CREATE PROCEDURE [dbo].[DBG_RegisterIISCall]
	@in_API	varchar(128),
	@in_BytesIn int,
	@in_BytesOut int
AS
BEGIN
	SET NOCOUNT ON;

	select 0 as ResultCode
	/*
	update DBG_IISApiStats set Hits=Hits+1, BytesIn=BytesIn+@in_BytesIn, BytesOut=BytesOut+@in_BytesOut where API=@in_API
	if(@@ROWCOUNT = 0) begin
		insert into DBG_IISApiStats	values (
			@in_API,
			1,
			@in_BytesIn,
			@in_BytesOut
		)
	end
	*/

	return
END
GO


-- ----------------------------
-- procedure structure for FN_ADD_SECURITY_LOG
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[FN_ADD_SECURITY_LOG]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[FN_ADD_SECURITY_LOG]
GO

CREATE PROCEDURE [dbo].[FN_ADD_SECURITY_LOG]
	-- Add the parameters for the stored procedure here
	@EventID int,
	@IP varchar(64),
	@CustomerID int,
	@EventData varchar(256)
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

	INSERT INTO SecurityLog
		(EventID, Date, IP, CustomerID, EventData)
	VALUES
		(@EventID, GETDATE(), @IP, @CustomerID, @EventData)

END
GO


-- ----------------------------
-- procedure structure for FN_AddItemToUser
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[FN_AddItemToUser]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[FN_AddItemToUser]
GO

CREATE PROCEDURE [dbo].[FN_AddItemToUser]
	@in_CustomerID int,
	@in_ItemID int,
	@in_ExpDays int
AS
BEGIN
	SET NOCOUNT ON;

	declare @InventoryID bigint = 0
	declare @LeasedUntil datetime
	declare @CurDate datetime = GETDATE()

	-- check if this is stackable item, if so - get buying stack size.
	-- stackable item defined where NumClips>0, Quantity is ClipSize
	declare @BuyStackSize int = 1
	select @BuyStackSize=ClipSize from Items_Weapons where ItemID=@in_ItemID and NumClips>0

	-- see if we already have that item in inventory without modification vars
	select @InventoryID=InventoryID, @LeasedUntil=LeasedUntil from UsersInventory
		where (CustomerID=@in_CustomerID and CharID=0 and ItemID=@in_ItemID and Var1<0)
	if(@InventoryID = 0)
	begin
		INSERT INTO UsersInventory (
			CustomerID,
			CharID,
			ItemID,
			LeasedUntil,
			Quantity
		)
		VALUES (
			@in_CustomerID,
			0,
			@in_ItemID,
			DATEADD(day, @in_ExpDays, @CurDate),
			@BuyStackSize
		)
		return
	end

	if(@LeasedUntil < @CurDate)
		set @LeasedUntil = DATEADD(day, @in_ExpDays, @CurDate)
	else
		set @LeasedUntil = DATEADD(day, @in_ExpDays, @LeasedUntil)

	if(@LeasedUntil > '2020-1-1')
		set @LeasedUntil = '2020-1-1'

	-- all items is stackable by default
	UPDATE UsersInventory SET
		LeasedUntil=@LeasedUntil,
		Quantity=(Quantity+@BuyStackSize)
	WHERE InventoryID=@InventoryID

	return
END
GO


-- ----------------------------
-- procedure structure for FN_AlterUserGP
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[FN_AlterUserGP]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[FN_AlterUserGP]
GO

CREATE PROCEDURE [dbo].[FN_AlterUserGP]
	@in_CustomerID int,
	@in_GP int,
	@in_Reason varchar(64)
AS
BEGIN
	SET NOCOUNT ON;

	if(@in_GP = 0)
		return

	declare @GamePoints int = 0
	select @GamePoints=GamePoints from UsersData where CustomerID=@in_CustomerID

	insert into DBG_GPTransactions (
		CustomerID,
		TransactionTime,
		Amount,
		Reason,
		Previous
	) values (
		@in_CustomerID,
		GETDATE(),
		@in_GP,
		@in_Reason,
		@GamePoints
	)

	update UsersData set GamePoints=(GamePoints + @in_GP) where CustomerID=@in_CustomerID

END
GO


-- ----------------------------
-- procedure structure for FN_BackpackValidateItem
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[FN_BackpackValidateItem]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[FN_BackpackValidateItem]
GO

CREATE PROCEDURE [dbo].[FN_BackpackValidateItem]
	@in_CharID int,
	@in_ItemID int,
	@in_EquipIdx int
AS
BEGIN
	SET NOCOUNT ON;

	if @in_ItemID = 0
		return 0

	if not exists (SELECT ItemID from UsersBackpack WHERE CharID=@in_CharID and ItemID=@in_ItemID and LeasedUntil>GETDATE()) begin
		return 0
	end

	-- item is valid
	return @in_ItemID
END
GO


-- ----------------------------
-- procedure structure for FN_CreateMD5Password
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[FN_CreateMD5Password]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[FN_CreateMD5Password]
GO

CREATE PROCEDURE [dbo].[FN_CreateMD5Password]
	@in_Password varchar(100),
	@out_MD5 varchar(32) OUTPUT
AS
BEGIN
	SET NOCOUNT ON;

	declare @PASSWORD_SALT varchar(100) = 'g5sc4gs1fz0h'
	set @out_MD5 = SUBSTRING(master.dbo.fn_varbintohexstr(HashBytes('md5', @PASSWORD_SALT + @in_Password)), 3, 999)
END
GO


-- ----------------------------
-- procedure structure for FN_LevelUpBonus
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[FN_LevelUpBonus]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[FN_LevelUpBonus]
GO

CREATE PROCEDURE [dbo].[FN_LevelUpBonus]
       @in_CustomerID int,
       @in_LevelUp int
AS
BEGIN
	SET NOCOUNT ON;

	declare @gd int = 0 -- level up bonus
	declare @sp int = 1 -- always give at least one SP

	-- not implemented yet

END
GO


-- ----------------------------
-- procedure structure for sp_alterdiagram
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[sp_alterdiagram]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[sp_alterdiagram]
GO

CREATE PROCEDURE [dbo].[sp_alterdiagram]
	(
		@diagramname 	sysname,
		@owner_id	int	= null,
		@version 	int,
		@definition 	varbinary(max)
	)
	WITH EXECUTE AS 'dbo'
	AS
	BEGIN
		set nocount on

		declare @theId 			int
		declare @retval 		int
		declare @IsDbo 			int

		declare @UIDFound 		int
		declare @DiagId			int
		declare @ShouldChangeUID	int

		if(@diagramname is null)
		begin
			RAISERROR ('Invalid ARG', 16, 1)
			return -1
		end

		execute as caller;
		select @theId = DATABASE_PRINCIPAL_ID();
		select @IsDbo = IS_MEMBER(N'db_owner');
		if(@owner_id is null)
			select @owner_id = @theId;
		revert;

		select @ShouldChangeUID = 0
		select @DiagId = diagram_id, @UIDFound = principal_id from dbo.sysdiagrams where principal_id = @owner_id and name = @diagramname

		if(@DiagId IS NULL or (@IsDbo = 0 and @theId <> @UIDFound))
		begin
			RAISERROR ('Diagram does not exist or you do not have permission.', 16, 1);
			return -3
		end

		if(@IsDbo <> 0)
		begin
			if(@UIDFound is null or USER_NAME(@UIDFound) is null) -- invalid principal_id
			begin
				select @ShouldChangeUID = 1 ;
			end
		end

		-- update dds data
		update dbo.sysdiagrams set definition = @definition where diagram_id = @DiagId ;

		-- change owner
		if(@ShouldChangeUID = 1)
			update dbo.sysdiagrams set principal_id = @theId where diagram_id = @DiagId ;

		-- update dds version
		if(@version is not null)
			update dbo.sysdiagrams set version = @version where diagram_id = @DiagId ;

		return 0
	END
GO


-- ----------------------------
-- procedure structure for sp_creatediagram
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[sp_creatediagram]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[sp_creatediagram]
GO

CREATE PROCEDURE [dbo].[sp_creatediagram]
	(
		@diagramname 	sysname,
		@owner_id		int	= null,
		@version 		int,
		@definition 	varbinary(max)
	)
	WITH EXECUTE AS 'dbo'
	AS
	BEGIN
		set nocount on

		declare @theId int
		declare @retval int
		declare @IsDbo	int
		declare @userName sysname
		if(@version is null or @diagramname is null)
		begin
			RAISERROR (N'E_INVALIDARG', 16, 1);
			return -1
		end

		execute as caller;
		select @theId = DATABASE_PRINCIPAL_ID();
		select @IsDbo = IS_MEMBER(N'db_owner');
		revert;

		if @owner_id is null
		begin
			select @owner_id = @theId;
		end
		else
		begin
			if @theId <> @owner_id
			begin
				if @IsDbo = 0
				begin
					RAISERROR (N'E_INVALIDARG', 16, 1);
					return -1
				end
				select @theId = @owner_id
			end
		end
		-- next 2 line only for test, will be removed after define name unique
		if EXISTS(select diagram_id from dbo.sysdiagrams where principal_id = @theId and name = @diagramname)
		begin
			RAISERROR ('The name is already used.', 16, 1);
			return -2
		end

		insert into dbo.sysdiagrams(name, principal_id , version, definition)
				VALUES(@diagramname, @theId, @version, @definition) ;

		select @retval = @@IDENTITY
		return @retval
	END
GO


-- ----------------------------
-- procedure structure for sp_dropdiagram
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[sp_dropdiagram]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[sp_dropdiagram]
GO

CREATE PROCEDURE [dbo].[sp_dropdiagram]
	(
		@diagramname 	sysname,
		@owner_id	int	= null
	)
	WITH EXECUTE AS 'dbo'
	AS
	BEGIN
		set nocount on
		declare @theId 			int
		declare @IsDbo 			int

		declare @UIDFound 		int
		declare @DiagId			int

		if(@diagramname is null)
		begin
			RAISERROR ('Invalid value', 16, 1);
			return -1
		end

		EXECUTE AS CALLER;
		select @theId = DATABASE_PRINCIPAL_ID();
		select @IsDbo = IS_MEMBER(N'db_owner');
		if(@owner_id is null)
			select @owner_id = @theId;
		REVERT;

		select @DiagId = diagram_id, @UIDFound = principal_id from dbo.sysdiagrams where principal_id = @owner_id and name = @diagramname
		if(@DiagId IS NULL or (@IsDbo = 0 and @UIDFound <> @theId))
		begin
			RAISERROR ('Diagram does not exist or you do not have permission.', 16, 1)
			return -3
		end

		delete from dbo.sysdiagrams where diagram_id = @DiagId;

		return 0;
	END
GO


-- ----------------------------
-- procedure structure for sp_helpdiagramdefinition
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[sp_helpdiagramdefinition]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[sp_helpdiagramdefinition]
GO

CREATE PROCEDURE [dbo].[sp_helpdiagramdefinition]
	(
		@diagramname 	sysname,
		@owner_id	int	= null
	)
	WITH EXECUTE AS N'dbo'
	AS
	BEGIN
		set nocount on

		declare @theId 		int
		declare @IsDbo 		int
		declare @DiagId		int
		declare @UIDFound	int

		if(@diagramname is null)
		begin
			RAISERROR (N'E_INVALIDARG', 16, 1);
			return -1
		end

		execute as caller;
		select @theId = DATABASE_PRINCIPAL_ID();
		select @IsDbo = IS_MEMBER(N'db_owner');
		if(@owner_id is null)
			select @owner_id = @theId;
		revert;

		select @DiagId = diagram_id, @UIDFound = principal_id from dbo.sysdiagrams where principal_id = @owner_id and name = @diagramname;
		if(@DiagId IS NULL or (@IsDbo = 0 and @UIDFound <> @theId ))
		begin
			RAISERROR ('Diagram does not exist or you do not have permission.', 16, 1);
			return -3
		end

		select version, definition FROM dbo.sysdiagrams where diagram_id = @DiagId ;
		return 0
	END
GO


-- ----------------------------
-- procedure structure for sp_helpdiagrams
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[sp_helpdiagrams]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[sp_helpdiagrams]
GO

CREATE PROCEDURE [dbo].[sp_helpdiagrams]
	(
		@diagramname sysname = NULL,
		@owner_id int = NULL
	)
	WITH EXECUTE AS N'dbo'
	AS
	BEGIN
		DECLARE @user sysname
		DECLARE @dboLogin bit
		EXECUTE AS CALLER;
			SET @user = USER_NAME();
			SET @dboLogin = CONVERT(bit,IS_MEMBER('db_owner'));
		REVERT;
		SELECT
			[Database] = DB_NAME(),
			[Name] = name,
			[ID] = diagram_id,
			[Owner] = USER_NAME(principal_id),
			[OwnerID] = principal_id
		FROM
			sysdiagrams
		WHERE
			(@dboLogin = 1 OR USER_NAME(principal_id) = @user) AND
			(@diagramname IS NULL OR name = @diagramname) AND
			(@owner_id IS NULL OR principal_id = @owner_id)
		ORDER BY
			4, 5, 1
	END
GO


-- ----------------------------
-- procedure structure for sp_renamediagram
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[sp_renamediagram]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[sp_renamediagram]
GO

CREATE PROCEDURE [dbo].[sp_renamediagram]
	(
		@diagramname 		sysname,
		@owner_id		int	= null,
		@new_diagramname	sysname

	)
	WITH EXECUTE AS 'dbo'
	AS
	BEGIN
		set nocount on
		declare @theId 			int
		declare @IsDbo 			int

		declare @UIDFound 		int
		declare @DiagId			int
		declare @DiagIdTarg		int
		declare @u_name			sysname
		if((@diagramname is null) or (@new_diagramname is null))
		begin
			RAISERROR ('Invalid value', 16, 1);
			return -1
		end

		EXECUTE AS CALLER;
		select @theId = DATABASE_PRINCIPAL_ID();
		select @IsDbo = IS_MEMBER(N'db_owner');
		if(@owner_id is null)
			select @owner_id = @theId;
		REVERT;

		select @u_name = USER_NAME(@owner_id)

		select @DiagId = diagram_id, @UIDFound = principal_id from dbo.sysdiagrams where principal_id = @owner_id and name = @diagramname
		if(@DiagId IS NULL or (@IsDbo = 0 and @UIDFound <> @theId))
		begin
			RAISERROR ('Diagram does not exist or you do not have permission.', 16, 1)
			return -3
		end

		-- if((@u_name is not null) and (@new_diagramname = @diagramname))	-- nothing will change
		--	return 0;

		if(@u_name is null)
			select @DiagIdTarg = diagram_id from dbo.sysdiagrams where principal_id = @theId and name = @new_diagramname
		else
			select @DiagIdTarg = diagram_id from dbo.sysdiagrams where principal_id = @owner_id and name = @new_diagramname

		if((@DiagIdTarg is not null) and  @DiagId <> @DiagIdTarg)
		begin
			RAISERROR ('The name is already used.', 16, 1);
			return -2
		end

		if(@u_name is null)
			update dbo.sysdiagrams set [name] = @new_diagramname, principal_id = @theId where diagram_id = @DiagId
		else
			update dbo.sysdiagrams set [name] = @new_diagramname where diagram_id = @DiagId
		return 0
	END
GO


-- ----------------------------
-- procedure structure for sp_upgraddiagrams
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[sp_upgraddiagrams]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[sp_upgraddiagrams]
GO

CREATE PROCEDURE [dbo].[sp_upgraddiagrams]
	AS
	BEGIN
		IF OBJECT_ID(N'dbo.sysdiagrams') IS NOT NULL
			return 0;

		CREATE TABLE dbo.sysdiagrams
		(
			name sysname NOT NULL,
			principal_id int NOT NULL,	-- we may change it to varbinary(85)
			diagram_id int PRIMARY KEY IDENTITY,
			version int,

			definition varbinary(max)
			CONSTRAINT UK_principal_name UNIQUE
			(
				principal_id,
				name
			)
		);


		/* Add this if we need to have some form of extended properties for diagrams */
		/*
		IF OBJECT_ID(N'dbo.sysdiagram_properties') IS NULL
		BEGIN
			CREATE TABLE dbo.sysdiagram_properties
			(
				diagram_id int,
				name sysname,
				value varbinary(max) NOT NULL
			)
		END
		*/

		IF OBJECT_ID(N'dbo.dtproperties') IS NOT NULL
		begin
			insert into dbo.sysdiagrams
			(
				[name],
				[principal_id],
				[version],
				[definition]
			)
			select
				convert(sysname, dgnm.[uvalue]),
				DATABASE_PRINCIPAL_ID(N'dbo'),			-- will change to the sid of sa
				0,							-- zero for old format, dgdef.[version],
				dgdef.[lvalue]
			from dbo.[dtproperties] dgnm
				inner join dbo.[dtproperties] dggd on dggd.[property] = 'DtgSchemaGUID' and dggd.[objectid] = dgnm.[objectid]
				inner join dbo.[dtproperties] dgdef on dgdef.[property] = 'DtgSchemaDATA' and dgdef.[objectid] = dgnm.[objectid]

			where dgnm.[property] = 'DtgSchemaNAME' and dggd.[uvalue] like N'_EA3E6268-D998-11CE-9454-00AA00A3F36E_'
			return 2;
		end
		return 1;
	END
GO


-- ----------------------------
-- procedure structure for TEMP_AddGPToUser
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[TEMP_AddGPToUser]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[TEMP_AddGPToUser]
GO

CREATE PROCEDURE [dbo].[TEMP_AddGPToUser]
	@in_email varchar(128),
	@in_GP int
AS
BEGIN
	SET NOCOUNT ON;

	--
	-- used in account.thewarz.com/admin/gpadd
	--

	declare @CustomerID int = 0
	select @CustomerID=CustomerID from dbo.Accounts where email=@in_email
	if(@@ROWCOUNT = 0) begin
		select 0 as 'CustomerID'
		return
	end

	declare @GamePoints int = 0
	select @GamePoints=GamePoints from UsersData where CustomerID=@CustomerID

	insert into DBG_GPTransactions (
		CustomerID,
		TransactionTime,
		Amount,
		Reason,
		Previous
	) values (
		@CustomerID,
		GETDATE(),
		@in_GP,
		'ADMIN_ADD',
		@GamePoints
	)

	update UsersData set GamePoints=(GamePoints + @in_GP) where CustomerID=@CustomerID
	select @CustomerID as 'CustomerID', (@GamePoints+@in_GP) as 'GamePoints'

END
GO


-- ----------------------------
-- procedure structure for WZ_ACCOUNT_APPLYKEY
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ACCOUNT_APPLYKEY]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ACCOUNT_APPLYKEY]
GO

CREATE PROCEDURE [dbo].[WZ_ACCOUNT_APPLYKEY]
	@in_CustomerID int,
	@in_SerialKey varchar(128)
AS
BEGIN
	SET NOCOUNT ON;

	declare @AccountType int = -1
	select @AccountType=AccountType from dbo.UsersData where CustomerID=@in_CustomerID
	if(@@ROWCOUNT = 0) begin
		select 6 as ResultCode, 'no user' as ResultMsg;
		return
	end

	-- only guest accounts can be extended
	if(@AccountType <> 3) begin
		select 2 as ResultCode, 'bad account type' as ResultMsg;
		return
	end

	--
	-- add new return codes in CUpdater::DoApplyNewKey
	--

-- check for serial key
	declare @keyResultCode int = 99
	declare @keyCustomerID int = 99
	declare @keySerialType int = 99
	exec [BreezeNet].[dbo].[BN_WarZ_SerialGetInfo]
		@in_SerialKey,
		'email@not.used',
		@keyResultCode out,
		@keyCustomerID out,
		@keySerialType out

	if(@keyResultCode <> 0) begin
		select 3 as ResultCode, 'Serial not valid' as ResultMsg;
		return
	end
	if(@keyCustomerID > 0) begin
		select 4 as ResultCode, 'Serial already used' as ResultMsg;
		return
	end

-- update account type and expiration time
	declare @DateActiveUntil datetime = '2030-1-1'
	if(@keySerialType = 3) begin
		-- guest accounts have 48hrs play time (sync with [WZ_ACCOUNT_CREATE])
		set @DateActiveUntil = DATEADD(hour, 48, GETDATE())
	end
	update UsersData set DateActiveUntil=@DateActiveUntil, AccountType=@keySerialType where CustomerID=@in_CustomerID

-- register CustomerID in BreezeNet
	exec [BreezeNet].[dbo].[BN_WarZ_SerialSetCustomerID] @in_SerialKey, @in_CustomerID

-- BONUSES from [WZ_ACCOUNT_CREATE] - do not forget to sync them
	if(@keySerialType = 0) begin
		-- legend package, 30$ 1GC=142
		update UsersData set GamePoints=(GamePoints+4260) where CustomerID=@in_CustomerID
	end

	if(@keySerialType = 1) begin
		-- pioneer package, 15$ 1GC=142
		update UsersData set GamePoints=(GamePoints+2139) where CustomerID=@in_CustomerID
	end

	-- success
	select 0 as ResultCode
	select @in_CustomerID as CustomerID, @keySerialType as 'AccountType'

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ACCOUNT_ChangePassword
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ACCOUNT_ChangePassword]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ACCOUNT_ChangePassword]
GO

CREATE PROCEDURE [dbo].[WZ_ACCOUNT_ChangePassword]
	@in_CustomerID int,
	@in_NewPassword varchar(100)
AS
BEGIN
	SET NOCOUNT ON;

	-- update new password
	declare @MD5FromPwd varchar(100)
	exec FN_CreateMD5Password @in_NewPassword, @MD5FromPwd OUTPUT
	update Accounts set MD5Password=@MD5FromPwd where CustomerID=@in_CustomerID

	insert into DBG_PasswordResets (
		CustomerID,
		ResetDate,
		NewPassword
	) values (
		@in_CustomerID,
		GETDATE(),
		'' --@in_NewPassword Removed DO NOT store password as plain text ! ;/
	)

	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_ACCOUNT_DELETE
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ACCOUNT_DELETE]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ACCOUNT_DELETE]
GO

CREATE PROCEDURE [dbo].[WZ_ACCOUNT_DELETE]
	@in_CustomerID int
AS
BEGIN
	SET NOCOUNT ON;

	-- success
	select 0 as ResultCode

	delete from dbo.UsersChars where CustomerID=@in_CustomerID
	delete from dbo.UsersData where CustomerID=@in_CustomerID
	delete from dbo.UsersInventory where CustomerID=@in_CustomerID

	update dbo.Accounts set AccountStatus=999 where CustomerID=@in_CustomerID

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ACCOUNT_LOGIN
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ACCOUNT_LOGIN]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ACCOUNT_LOGIN]
GO

CREATE PROCEDURE [dbo].[WZ_ACCOUNT_LOGIN]
	@in_IP varchar(100),
	@in_EMail varchar(100),
	@in_Password varchar(100)
AS
BEGIN
	SET NOCOUNT ON;

	declare @CustomerID int
	declare @MD5Password varchar(100)
	declare @AccountStatus int = 0	-- this is Accounts.AccountStatus

	-- this call is always valid
	select 0 as ResultCode

	-- search for record with username
	SELECT
		@CustomerID=CustomerID,
		@MD5Password=MD5Password,
		@AccountStatus=AccountStatus
	FROM Accounts
	WHERE email=@in_Email
	if (@@RowCount = 0) begin
		select
			1 as LoginResult,
			0 as CustomerID,
			0 as AccountStatus
		return
	end

	-- check MD5 password
	declare @MD5FromPwd varchar(100)
	exec FN_CreateMD5Password @in_Password, @MD5FromPwd OUTPUT
	if(@MD5Password <> @MD5FromPwd) begin
		select
			2 as LoginResult,
			0 as CustomerID,
			0 as AccountStatus
		return
	end

	-- check if deleted account because of refund
	if(@AccountStatus = 999) begin
		select
			3 as LoginResult,
			0 as CustomerID,
			999 as AccountStatus
		return
	end

	-- perform actual login
	exec WZ_ACCOUNT_LOGIN_EXEC @in_IP, @CustomerID
END
GO


-- ----------------------------
-- procedure structure for WZ_ACCOUNT_LOGIN_EXEC
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ACCOUNT_LOGIN_EXEC]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ACCOUNT_LOGIN_EXEC]
GO

CREATE PROCEDURE [dbo].[WZ_ACCOUNT_LOGIN_EXEC]
	@in_IP varchar(100),
	@CustomerID int
AS
BEGIN
	SET NOCOUNT ON;

	--
	-- helper function that perform actual user login
	--

	declare @IsDeveloper int = 0
	declare @AccountStatus int
	declare @DateActiveUntil datetime
	declare @BanExpireDate datetime
	declare @lastgamedate datetime
	declare @GameServerId int
	select
		@AccountStatus=AccountStatus,
		@IsDeveloper=IsDeveloper,
		@DateActiveUntil=DateActiveUntil,
		@BanExpireDate=BanExpireDate,
		@GameServerId=GameServerId,
		@lastgamedate=lastgamedate
		from UsersData where CustomerID=@CustomerID
	if(@@ROWCOUNT = 0) begin
		select
			5 as LoginResult,
			@CustomerID as CustomerID,
			0 as AccountStatus,
			0 as SessionID,
			0 as IsDeveloper
		return
	end

	-- check if account time expired
	if(GETDATE() > @DateActiveUntil) begin
		select
			4 as LoginResult,
			@CustomerID as CustomerID,
			300 as AccountStatus,	-- special 'TimeExpired' code
			0 as SessionID,
			0 as IsDeveloper
		return
	end

	-- status equal to 201 means temporary ban
	if (@AccountStatus = 201) begin
		declare @BanExpireMin int = DATEDIFF(mi, GETDATE(), @BanExpireDate)
		if(@BanExpireMin > 0) begin
			select
				3 as LoginResult,
				@CustomerID as CustomerID,
				@AccountStatus as AccountStatus,
				@BanExpireMin as SessionID,
				0 as IsDeveloper
			return
		end
		else
		begin
			-- unban player
			set @AccountStatus = 100
			update dbo.UsersData set AccountStatus=@AccountStatus where CustomerID=@CustomerID
		end
	end

	if (@AccountStatus >= 200) begin
		select
			3 as LoginResult,
			@CustomerID as CustomerID,
			@AccountStatus as AccountStatus,
			0 as SessionID,
			0 as IsDeveloper
		return
	end

	-- check if game is still active or 90sec passed from last update (COPYPASTE_GAMECHECK, search for others)
	if(@GameServerId > 0 and DATEDIFF(second, @lastgamedate, GETDATE()) < 90) begin
		select
			0 as LoginResult,
			@CustomerID as CustomerID,
			70 as AccountStatus,	-- game still active code
			0 as SessionID,
			0 as IsDeveloper
		return
	end

	-- update session key/id
	declare @SessionKey varchar(50) = NEWID()
	declare @SessionID int = checksum(@SessionKey)
	if exists (SELECT CustomerID FROM LoginSessions WHERE CustomerID = @CustomerID)
	begin
		UPDATE LoginSessions SET
			SessionKey=@SessionKey,
			SessionID=@SessionID,
			LoginIP=@in_IP,
			TimeLogged=GETDATE(),
			TimeUpdated=GETDATE()
		WHERE CustomerID=@CustomerID
	end
	else
	begin
		INSERT INTO LoginSessions
			(CustomerID, SessionKey, SessionID, LoginIP, TimeLogged, TimeUpdated)
		VALUES
			(@CustomerID, @SessionKey, @SessionID, @in_IP, GETDATE(), GETDATE())
	end

	-- update other tables
	UPDATE Accounts SET
		lastlogindate=GETDATE(),
		lastloginIP=@in_IP
	WHERE CustomerID=@CustomerID

	INSERT INTO Logins
		(CustomerID, LoginTime, IP, LoginSource)
	VALUES
		(@CustomerID, GETDATE(), @in_IP, 0)

	-- return session info
	SELECT
		0 as LoginResult,
		@CustomerID as CustomerID,
		@AccountStatus as AccountStatus,
		@SessionID as SessionID,
		@IsDeveloper as IsDeveloper
END
GO


-- ----------------------------
-- procedure structure for WZ_Backpack_SRV_AddItem
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_Backpack_SRV_AddItem]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_Backpack_SRV_AddItem]
GO

CREATE PROCEDURE [dbo].[WZ_Backpack_SRV_AddItem]
	@in_CustomerID int,
	@in_CharID int,
	@in_Slot int,
	@in_ItemID int,
	@in_Amount int,
	@in_Var1 int,
	@in_Var2 int
AS
BEGIN
	SET NOCOUNT ON;

	-- sanity check: input
	if(@in_ItemID = 0) begin
		select 6 as ResultCode, 'add item failed#1' as ResultMsg
		return
	end

	-- sanity check, we must not have item in that slot
	declare @InventoryID bigint = 0
	select @InventoryID=InventoryID from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_Slot
	if(@InventoryID > 0) begin
		select 6 as ResultCode, 'add item failed#2' as ResultMsg
		return
	end

	INSERT INTO UsersInventory (
		CustomerID,
		CharID,
		BackpackSlot,
		ItemID,
		LeasedUntil,
		Quantity,
		Var1,
		Var2
	)
	VALUES (
		@in_CustomerID,
		@in_CharID,
		@in_Slot,
		@in_ItemID,
		DATEADD(day, 2000, GETDATE()),
		@in_Amount,
		@in_Var1,
		@in_Var2
	)
	set @InventoryID = SCOPE_IDENTITY()

	select 0 as ResultCode
	select @InventoryID as 'InventoryID'
END
GO


-- ----------------------------
-- procedure structure for WZ_Backpack_SRV_AlterItem
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_Backpack_SRV_AlterItem]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_Backpack_SRV_AlterItem]
GO

CREATE PROCEDURE [dbo].[WZ_Backpack_SRV_AlterItem]
	@in_CustomerID int,
	@in_CharID int,
	@in_Slot int,
	@in_ItemID int,
	@in_Amount int,
	@in_Var1 int,
	@in_Var2 int
AS
BEGIN
	SET NOCOUNT ON;

	-- sanity check: input
	if(@in_ItemID = 0) begin
		select 6 as ResultCode, 'add item failed#1' as ResultMsg
		return
	end

	update UsersInventory set
		ItemID=@in_ItemID,
		Quantity=@in_Amount,
		Var1=@in_Var1,
		Var2=@in_Var2
	where CharID=@in_CharID and BackpackSlot=@in_Slot

	if(@@ROWCOUNT = 0) begin
		select 6 as ResultCode, 'alter item failed' as ResultMsg
		return
	end

	select 0 as ResultCode
	return

END
GO


-- ----------------------------
-- procedure structure for WZ_Backpack_SRV_Change
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_Backpack_SRV_Change]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_Backpack_SRV_Change]
GO

CREATE PROCEDURE [dbo].[WZ_Backpack_SRV_Change]
	@in_CustomerID int,
	@in_CharID int,
	@in_BackpackID int,
	@in_BackpackSize int
AS
BEGIN
	SET NOCOUNT ON;

	--
	-- _SRV_ function - no validity checks
	--

	-- replace backpack size/id
	update UsersChars set BackpackID=@in_BackpackID, BackpackSize=@in_BackpackSize where CharID=@in_CharID

	select 0 as ResultCode
	select 0 as 'InventoryID'
END
GO


-- ----------------------------
-- procedure structure for WZ_Backpack_SRV_DeleteItem
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_Backpack_SRV_DeleteItem]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_Backpack_SRV_DeleteItem]
GO

CREATE PROCEDURE [dbo].[WZ_Backpack_SRV_DeleteItem]
	@in_CustomerID int,
	@in_CharID int,
	@in_Slot int,
	@in_ItemID int = 0, -- not used
	@in_Amount int = 0, -- not used
	@in_Var1 int = 0, -- not used
	@in_Var2 int = 0 -- not used
AS
BEGIN
	SET NOCOUNT ON;

	delete from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_Slot
	if(@@ROWCOUNT = 0) begin
		select 6 as ResultCode, 'delete item failed' as ResultMsg
		return
	end

	select 0 as ResultCode
	return

END
GO


-- ----------------------------
-- procedure structure for WZ_BackpackChange
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BackpackChange]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BackpackChange]
GO

CREATE PROCEDURE [dbo].[WZ_BackpackChange]
	@in_CustomerID int,
	@in_CharID int,
	@in_InventoryID bigint
AS
BEGIN
	SET NOCOUNT ON;

	-- check if CustomerID/CharID pair is valid
	declare @CustomerID int
	declare @BackpackSize int
	declare @GameFlags int
	select @CustomerID=CustomerID, @BackpackSize=BackpackSize, @GameFlags=GameFlags FROM UsersChars WHERE CharID=@in_CharID
	if(@@ROWCOUNT = 0 or @CustomerID <> @in_CustomerID) begin
		select 6 as ResultCode, 'bad charid' as ResultMsg
		return
	end

	-- do not allow operations outside safe zone
	if((@GameFlags & 1) = 0) begin
		select 9 as ResultCode, 'outside safe zone' as ResultMsg
		return
	end

	-- check if game is still active or 90sec passed from last update (COPYPASTE_GAMECHECK, search for others)
	declare @lastgamedate datetime
	declare @GameServerId int
	select @GameServerId=GameServerId, @lastgamedate=lastgamedate from UsersData where CustomerID=@in_CustomerID
	if(@GameServerId > 0 and DATEDIFF(second, @lastgamedate, GETDATE()) < 90) begin
		select 7 as ResultCode, 'game still active' as ResultMsg
		return
	end

	-- validate that we own that item
	declare @InvItemID int = 0
	declare @InvCustomerID int = 0
	declare @InvCharID int = 0
	declare @InvQuantity int = 0
	select @InvItemID=ItemID, @InvQuantity=Quantity, @InvCustomerID=CustomerID from UsersInventory where InventoryID=@in_InventoryID
	if(@InvCustomerID <> @in_CustomerID) begin
		select 6 as ResultCode, 'bad inventoryid #1' as ResultMsg
		return
	end
	if(@InvCharID > 0 and @InvCharID <> @in_CharID) begin
		select 6 as ResultCode, 'bad inventoryid #2' as ResultMsg
		return
	end

	-- validate that this is actually a backpack
	declare @MaxSlots int = 0
	select @MaxSlots=Bulkiness from Items_Gear where ItemID=@InvItemID and Category=12
	if(@MaxSlots = 0) begin
		select 6 as ResultCode, 'no backpack' as ResultMsg
		return
	end

	-- move everything above current slots to inventory
	update UsersInventory set CharID=0, BackpackSlot=-1 where CharID=@in_CharID and BackpackSlot>=@MaxSlots

	-- remove single backpack from inventory
	set @InvQuantity = @InvQuantity - 1
	if(@InvQuantity <= 0) begin
		delete from UsersInventory where InventoryID=@in_InventoryID
	end
	else begin
		update UsersInventory set Quantity=@InvQuantity where InventoryID=@in_InventoryID
	end

	-- place old backpack to inventory
	declare @OldBackpackID int
	select @OldBackpackID=BackpackID from UsersChars where CharID=@in_CharID
	exec dbo.FN_AddItemToUser @CustomerID, @OldBackpackID, 2000

	-- replace backpack size/id
	update UsersChars set BackpackID=@InvItemID, BackpackSize=@MaxSlots where CharID=@in_CharID

	select 0 as ResultCode
	select 0 as 'InventoryID'
END
GO


-- ----------------------------
-- procedure structure for WZ_BackpackFromInv
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BackpackFromInv]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BackpackFromInv]
GO

CREATE PROCEDURE [dbo].[WZ_BackpackFromInv]
	@in_CustomerID int,
	@in_CharID int,
	@in_InventoryID bigint,
	@in_Slot int,
	@in_Amount int
AS
BEGIN
	SET NOCOUNT ON;

	if(@in_Amount <= 0) begin
		select 6 as ResultCode, 'bad amount' as ResultMsg
		return
	end

	-- check if CustomerID/CharID pair is valid
	declare @CustomerID int
	declare @BackpackSize int
	select @CustomerID=CustomerID, @BackpackSize=BackpackSize FROM UsersChars WHERE CharID=@in_CharID
	if(@@ROWCOUNT = 0 or @CustomerID <> @in_CustomerID) begin
		select 6 as ResultCode, 'bad charid' as ResultMsg
		return
	end

	-- check if game is still active or 90sec passed from last update (COPYPASTE_GAMECHECK, search for others)
	declare @lastgamedate datetime
	declare @GameServerId int
	select @GameServerId=GameServerId, @lastgamedate=lastgamedate from UsersData where CustomerID=@in_CustomerID
	if(@GameServerId > 0 and DATEDIFF(second, @lastgamedate, GETDATE()) < 90) begin
		select 7 as ResultCode, 'game still active' as ResultMsg
		return
	end

	-- check if we have that item in inventory
	declare @InvCustomerID int
	declare @InvInventoryID bigint
	declare @InvItemID int
	declare @InvLeasedUntil datetime
	declare @InvQuantity int
	declare @InvVar1 int
	declare @InvVar2 int
	select
		@InvCustomerID=CustomerID,
		@InvInventoryID=InventoryID,
		@InvItemID=ItemID,
		@InvQuantity=Quantity,
		@InvLeasedUntil=LeasedUntil,
		@InvVar1=Var1,
		@InvVar2=Var2
	from UsersInventory where InventoryID=@in_InventoryID
	if(@@ROWCOUNT = 0 or @InvCustomerID <> @in_CustomerID) begin
		select 6 as ResultCode, 'bad inventoryid' as ResultMsg
		return
	end

	if(@in_Amount > @InvQuantity) begin
		select 6 as ResultCode, 'bad quantity' as ResultMsg
		return
	end

	-- validate backpack slot number
	if(@in_Slot < 0 or @in_Slot >= @BackpackSize) begin
		select 6 as ResultCode, 'bad slot' as ResultMsg
		return
	end
	-- validate itemid in that slot
	declare @BackpackInventoryID bigint = 0
	declare @BackpackItemID int = 0
	select
		@BackpackInventoryID=InventoryID,
		@BackpackItemID=ItemID
	from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_Slot
	if(@@ROWCOUNT > 0 and @BackpackItemID <> @InvItemID) begin
		select 6 as ResultCode, 'slot itemid mismatch' as ResultMsg
		return
	end

	-- clear attachments if moved item was in weapon slot
	if(@in_Slot = 0) update UsersChars set Attachment1='' where CharID=@in_CharID
	if(@in_Slot = 1) update UsersChars set Attachment2='' where CharID=@in_CharID

	-- check for easy case, unmodified item, no such item in backpack
	if(@BackpackInventoryID = 0 and @InvQuantity = @in_Amount) begin
		update UsersInventory set BackpackSlot=@in_Slot, CharID=@in_CharID where InventoryID=@InvInventoryID

		select 0 as ResultCode
		select @InvInventoryID as 'InventoryID'
		return
	end

	if(@BackpackInventoryID = 0)
	begin
		-- modified (won't stack) or new backpack item
		INSERT INTO UsersInventory (
			CustomerID,
			CharID,
			ItemID,
			BackpackSlot,
			LeasedUntil,
			Quantity,
			Var1,
			Var2
		)
		VALUES (
			@in_CustomerID,
			@in_CharID,
			@InvItemID,
			@in_Slot,
			@InvLeasedUntil,
			@in_Amount,
			@InvVar1,
			@InvVar2
		)
		set @BackpackInventoryID = SCOPE_IDENTITY()
	end
	else
	begin
		update UsersInventory set Quantity=(Quantity+@in_Amount) where InventoryID=@BackpackInventoryID
	end

	-- from inventory
	set @InvQuantity = @InvQuantity - @in_Amount
	if(@InvQuantity <= 0) begin
		delete from UsersInventory where InventoryID=@InvInventoryID
	end
	else begin
		update UsersInventory set Quantity=@InvQuantity where InventoryID=@InvInventoryID
	end

	select 0 as ResultCode;
	select @BackpackInventoryID as 'InventoryID'
END
GO


-- ----------------------------
-- procedure structure for WZ_BackpackGridJoin
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BackpackGridJoin]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BackpackGridJoin]
GO

CREATE PROCEDURE [dbo].[WZ_BackpackGridJoin]
	@in_CustomerID int,
	@in_CharID int,
	@in_SlotFrom int,
	@in_SlotTo int
AS
BEGIN
	SET NOCOUNT ON;

	-- check if CustomerID/CharID pair is valid
	declare @CustomerID int
	declare @BackpackSize int
	select @CustomerID=CustomerID, @BackpackSize=BackpackSize FROM UsersChars WHERE CharID=@in_CharID
	if(@@ROWCOUNT = 0 or @CustomerID <> @in_CustomerID) begin
		select 6 as ResultCode, 'bad charid' as ResultMsg
		return
	end

	-- check if game is still active or 90sec passed from last update (COPYPASTE_GAMECHECK, search for others)
	declare @lastgamedate datetime
	declare @GameServerId int
	select @GameServerId=GameServerId, @lastgamedate=lastgamedate from UsersData where CustomerID=@in_CustomerID
	if(@GameServerId > 0 and DATEDIFF(second, @lastgamedate, GETDATE()) < 90) begin
		select 7 as ResultCode, 'game still active' as ResultMsg
		return
	end

	-- check from slot
	declare @FromItemID int
	declare @FromQuantity int
	declare @FromVar1 int
	declare @FromVar2 int
	select
		@FromItemID=ItemID,
		@FromQuantity=Quantity,
		@FromVar1=Var1,
		@FromVar2=Var2
	from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_SlotFrom
	if(@@ROWCOUNT = 0 or @FromItemID = 0) begin
		select 6 as ResultCode, 'bad slot1' as ResultMsg
		return
	end

	-- check to slot
	declare @ToItemID int
	declare @ToQuantity int
	declare @ToVar1 int
	declare @ToVar2 int
	select
		@ToItemID=ItemID,
		@ToQuantity=Quantity,
		@ToVar1=Var1,
		@ToVar2=Var2
	from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_SlotTo
	if(@@ROWCOUNT = 0 or @ToItemID = 0) begin
		select 6 as ResultCode, 'bad slot2' as ResultMsg
		return
	end

	if(@ToItemID <> @FromItemID or @FromVar1 >= 0 or @ToVar1 >= 0) begin
		select 6 as ResultCode, 'bad join' as ResultMsg
		return
	end

	-- swap slots. operation will silently be ok if there is no item in that slot
	update UsersInventory set Quantity=Quantity+@FromQuantity where CharID=@in_CharID and BackpackSlot=@in_SlotTo
	delete from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_SlotFrom

	select 0 as ResultCode
	select 0 as 'InventoryID'
END
GO


-- ----------------------------
-- procedure structure for WZ_BackpackGridSwap
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BackpackGridSwap]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BackpackGridSwap]
GO

CREATE PROCEDURE [dbo].[WZ_BackpackGridSwap]
	@in_CustomerID int,
	@in_CharID int,
	@in_SlotFrom int,
	@in_SlotTo int
AS
BEGIN
	SET NOCOUNT ON;

	-- check if CustomerID/CharID pair is valid
	declare @CustomerID int
	declare @BackpackSize int
	select @CustomerID=CustomerID, @BackpackSize=BackpackSize FROM UsersChars WHERE CharID=@in_CharID
	if(@@ROWCOUNT = 0 or @CustomerID <> @in_CustomerID) begin
		select 6 as ResultCode, 'bad charid' as ResultMsg
		return
	end

	-- check if game is still active or 90sec passed from last update (COPYPASTE_GAMECHECK, search for others)
	declare @lastgamedate datetime
	declare @GameServerId int
	select @GameServerId=GameServerId, @lastgamedate=lastgamedate from UsersData where CustomerID=@in_CustomerID
	if(@GameServerId > 0 and DATEDIFF(second, @lastgamedate, GETDATE()) < 90) begin
		select 7 as ResultCode, 'game still active' as ResultMsg
		return
	end

	-- validate backpack slot number
	if(@in_SlotFrom < 0 or @in_SlotFrom >= @BackpackSize) begin
		select 6 as ResultCode, 'bad slot' as ResultMsg
		return
	end
	if(@in_SlotTo < 0 or @in_SlotTo >= @BackpackSize) begin
		select 6 as ResultCode, 'bad slot' as ResultMsg
		return
	end

	-- get inventory ids of both slots
	declare @InventoryIdFrom bigint = 0
	declare @InventoryIdTo bigint = 0
	select @InventoryIdFrom=InventoryID from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_SlotFrom
	select @InventoryIdTo=InventoryID   from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_SlotTo

	-- swap slots. operation will silently be ok if there is no item in that slot
	update UsersInventory set BackpackSlot=@in_SlotTo   where InventoryID=@InventoryIdFrom
	update UsersInventory set BackpackSlot=@in_SlotFrom where InventoryID=@InventoryIdTo

	-- clear attachments if swapped items was in weapon slots
	if(@in_SlotTo = 0 or @in_SlotFrom = 0) update UsersChars set Attachment1='' where CharID=@in_CharID
	if(@in_SlotTo = 1 or @in_SlotFrom = 1) update UsersChars set Attachment2='' where CharID=@in_CharID

	select 0 as ResultCode
	select 0 as 'InventoryID'
END
GO


-- ----------------------------
-- procedure structure for WZ_BackpackToInv
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BackpackToInv]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BackpackToInv]
GO

CREATE PROCEDURE [dbo].[WZ_BackpackToInv]
	@in_CustomerID int,
	@in_CharID int,
	@in_InventoryID bigint,	-- target inventory id where to put that item
	@in_Slot int,
	@in_Amount int
AS
BEGIN
	SET NOCOUNT ON;

	if(@in_Amount <= 0) begin
		select 6 as ResultCode, 'bad amount' as ResultMsg
		return
	end

	-- check if CustomerID/CharID pair is valid
	declare @CustomerID int
	select @CustomerID=CustomerID FROM UsersChars WHERE CharID=@in_CharID
	if(@@ROWCOUNT = 0 or @CustomerID <> @in_CustomerID) begin
		select 6 as ResultCode, 'bad charid' as ResultMsg
		return
	end

	-- check if game is still active or 90sec passed from last update (COPYPASTE_GAMECHECK, search for others)
	declare @lastgamedate datetime
	declare @GameServerId int
	select @GameServerId=GameServerId, @lastgamedate=lastgamedate from UsersData where CustomerID=@in_CustomerID
	if(@GameServerId > 0 and DATEDIFF(second, @lastgamedate, GETDATE()) < 90) begin
		select 7 as ResultCode, 'game still active' as ResultMsg
		return
	end

	declare @BackpackInventoryID bigint
	declare @BackpackItemID int
	declare @BackpackLeasedUntil datetime
	declare @BackpackQuantity int
	declare @BackpackVar1 int
	declare @BackpackVar2 int
	select
		@BackpackInventoryID=InventoryID,
		@BackpackItemID=ItemID,
		@BackpackQuantity=Quantity,
		@BackpackLeasedUntil=LeasedUntil,
		@BackpackVar1=Var1,
		@BackpackVar2=Var2
	from UsersInventory where CharID=@in_CharID and BackpackSlot=@in_Slot
	if(@@ROWCOUNT = 0) begin
		select 6 as ResultCode, 'bad slot' as ResultMsg
		return
	end

	if(@in_Amount > @BackpackQuantity) begin
		select 6 as ResultCode, 'bad quantity' as ResultMsg
		return
	end

	-- check for easy case, just switching to inventory
	if(@in_InventoryID = 0 and @BackpackQuantity = @in_Amount) begin
		update UsersInventory set BackpackSlot=-1, CharID=0 where InventoryID=@BackpackInventoryID

		select 0 as ResultCode
		select @BackpackInventoryID as 'InventoryID'
		return
	end

	-- validate that we own that inventory slot and item can be moved there
	if(@in_InventoryID > 0)
	begin
		declare @InvCustomerID int
		declare @InvCharID int
		declare @InvItemID int
		declare @InvVar1 int
		select
			@InvCustomerID=CustomerID,
			@InvCharID=CharID,
			@InvItemID=ItemID,
			@InvVar1=Var1
			from UsersInventory where InventoryID=@in_InventoryID
		if(@@ROWCOUNT = 0 or @InvCustomerID <> @in_CustomerID or @InvCharID <> 0 or @InvItemID <> @BackpackItemID) begin
			select 6 as ResultCode, 'bad inventoryid' as ResultMsg
			return
		end
		if(@InvVar1 >= 0 or @BackpackVar1 >= 0) begin
			select 6 as ResultCode, 'not stackable' as ResultMsg
			return
		end
	end

	declare @InvInventoryID bigint = @in_InventoryID
	if(@InvInventoryID = 0) begin
		-- modified (won't stack) or new inventory item
		INSERT INTO UsersInventory (
			CustomerID,
			CharID,
			ItemID,
			LeasedUntil,
			Quantity,
			Var1,
			Var2
		)
		VALUES (
			@in_CustomerID,
			0,
			@BackpackItemID,
			@BackpackLeasedUntil,
			@in_Amount,
			@BackpackVar1,
			@BackpackVar2
		)
		set @InvInventoryID = SCOPE_IDENTITY()
	end
	else begin
		update UsersInventory set Quantity=(Quantity+@in_Amount) where InventoryID=@InvInventoryID
	end

	-- from backpack
	set @BackpackQuantity = @BackpackQuantity - @in_Amount
	if(@BackpackQuantity <= 0) begin
		delete from UsersInventory where InventoryID=@BackpackInventoryID
	end
	else begin
		update UsersInventory set Quantity=@BackpackQuantity where InventoryID=@BackpackInventoryID
	end

	select 0 as ResultCode
	select @InvInventoryID as 'InventoryID'
END
GO


-- ----------------------------
-- procedure structure for WZ_BuyItem_GD
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BuyItem_GD]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BuyItem_GD]
GO

CREATE PROCEDURE [dbo].[WZ_BuyItem_GD]
	@in_IP char(32),
	@in_CustomerID int,
	@in_ItemId int,
	@in_BuyDays int
AS
BEGIN
	SET NOCOUNT ON;

	-- get points for that customer
	declare @GameDollars int
	SELECT @GameDollars=GameDollars FROM UsersData WHERE CustomerID=@in_CustomerID
	if (@@RowCount = 0) begin
		select 6 as ResultCode, 'no CustomerID' as ResultMsg
		return
	end

	declare @smsg1 varchar(1000)
	declare @out_FNResult int = 100

	-- get price
	declare @FinalPrice int
	exec WZ_BuyItemFN_GetPrice @out_FNResult out, @in_ItemId, @in_BuyDays, 'GD', @FinalPrice out
	if(@out_FNResult > 0) begin
		set @smsg1 = LTRIM(STR(@out_FNResult)) + ' GD '
		set @smsg1 = @smsg1 + LTRIM(STR(@in_BuyDays)) + ' ' + LTRIM(STR(@in_ItemID))
		EXEC FN_ADD_SECURITY_LOG 110, @in_IP, @in_CustomerID, @smsg1
		select 6 as ResultCode, 'bad GetPrice' as ResultMsg
		return
	end

	-- check if enough money
	if(@GameDollars < @FinalPrice) begin
		set @smsg1 = LTRIM(STR(@in_ItemId)) + ' ' + LTRIM(STR(@in_BuyDays)) + ' '
		set @smsg1 = @smsg1 + ' GD ' + LTRIM(STR(@FinalPrice)) + ' ' + LTRIM(STR(@GameDollars))
		EXEC FN_ADD_SECURITY_LOG 114, @in_IP, @in_CustomerID, @smsg1
		select 7 as ResultCode, 'Not Enough GD' as ResultMsg
		return
	end

	-- exec item adding function, if it fail, do not process transaction further
	exec WZ_BuyItemFN_Exec @out_FNResult out, @in_CustomerID, @in_ItemId, @in_BuyDays
	if(@out_FNResult <> 0) begin
		set @smsg1 = 'BuyExec failed' + LTRIM(STR(@out_FNResult))
		select 7 as ResultCode, @smsg1 as ResultMsg
		return
	end

	-- perform actual transaction
	set @GameDollars = @GameDollars-@FinalPrice;
	UPDATE UsersData SET GameDollars=@GameDollars where CustomerID=@in_CustomerID

	-- set transaction type
	declare @TType int = 0
	if(@in_BuyDays = 2000) set @TType = 3001;
	else set @TType = 2001;

	-- update transaction detail
	INSERT INTO FinancialTransactions
		VALUES (@in_CustomerID, 'INGAME', @TType, GETDATE(),
				@FinalPrice, '1', 'APPROVED', @in_ItemId)

	-- search for InventoryID of added item
	declare @InventoryID bigint = 0
	select @InventoryID=InventoryID from UsersInventory
		where CustomerID=@in_CustomerID and CharID=0 and ItemID=@in_ItemId and Var1<0

	select 0 as ResultCode
	select @GameDollars as 'Balance', @InventoryID as 'InventoryID'

END
GO


-- ----------------------------
-- procedure structure for WZ_BuyItem_GP
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BuyItem_GP]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BuyItem_GP]
GO

CREATE PROCEDURE [dbo].[WZ_BuyItem_GP]
	@in_IP char(32),
	@in_CustomerID int,
	@in_ItemId int,
	@in_BuyDays int
AS
BEGIN
	SET NOCOUNT ON;

	-- get points for that customer
	declare @GamePoints int
	SELECT @GamePoints=GamePoints FROM UsersData WHERE CustomerID=@in_CustomerID
	if (@@RowCount = 0) begin
		select 6 as ResultCode, 'no CustomerID' as ResultMsg
		return
	end

	declare @smsg1 varchar(1000)
	declare @out_FNResult int = 100

	-- get price
	declare @FinalPrice int
	exec WZ_BuyItemFN_GetPrice @out_FNResult out, @in_ItemId, @in_BuyDays, 'GP', @FinalPrice out
	if(@out_FNResult > 0) begin
		set @smsg1 = LTRIM(STR(@out_FNResult)) + ' GP '
		set @smsg1 = @smsg1 + LTRIM(STR(@in_BuyDays)) + ' ' + LTRIM(STR(@in_ItemID))
		EXEC FN_ADD_SECURITY_LOG 110, @in_IP, @in_CustomerID, @smsg1
		select 6 as ResultCode, 'bad GetPrice' as ResultMsg
		return
	end

	-- check if enough money
	if(@GamePoints < @FinalPrice) begin
		set @smsg1 = LTRIM(STR(@in_ItemId)) + ' ' + LTRIM(STR(@in_BuyDays)) + ' '
		set @smsg1 = @smsg1 + ' GP ' + LTRIM(STR(@FinalPrice)) + ' ' + LTRIM(STR(@GamePoints))
		EXEC FN_ADD_SECURITY_LOG 114, @in_IP, @in_CustomerID, @smsg1
		select 7 as ResultCode, 'Not Enough GP' as ResultMsg
		return
	end

	-- exec item adding function, if it fail, do not process transaction further
	exec WZ_BuyItemFN_Exec @out_FNResult out, @in_CustomerID, @in_ItemId, @in_BuyDays
	if(@out_FNResult <> 0) begin
		set @smsg1 = 'BuyExec failed' + LTRIM(STR(@out_FNResult))
		select 7 as ResultCode, @smsg1 as ResultMsg
		return
	end

	-- perform actual transaction
	declare @AlterGP int = -@FinalPrice;
	exec FN_AlterUserGP @in_CustomerID, @AlterGP, 'WZ_BuyItem_GP'
	set @GamePoints=@GamePoints-@FinalPrice;

	-- set transaction type
	declare @TType int = 0
	if(@in_BuyDays = 2000) set @TType = 3000;
	else set @TType = 2000;

	-- update transaction detail
	INSERT INTO FinancialTransactions
		VALUES (@in_CustomerID, 'INGAME', @TType, GETDATE(),
				@FinalPrice, '1', 'APPROVED', @in_ItemId)

	-- search for InventoryID of added item
	declare @InventoryID bigint = 0
	select @InventoryID=InventoryID from UsersInventory
		where CustomerID=@in_CustomerID and CharID=0 and ItemID=@in_ItemId and Var1<0

	select 0 as ResultCode
	select @GamePoints as 'Balance', @InventoryID as 'InventoryID';

END
GO


-- ----------------------------
-- procedure structure for WZ_BuyItemFN_Exec
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BuyItemFN_Exec]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BuyItemFN_Exec]
GO

CREATE PROCEDURE [dbo].[WZ_BuyItemFN_Exec]
	@out_FNResult int out,
	@in_CustomerID int,
	@in_ItemId int,
	@in_BuyDays int
AS
BEGIN
	SET NOCOUNT ON;

	--
	--
	-- main function for buying items in game, should be called from WZ_BuyItem2
	--
	--

	-- set success by default
	set @out_FNResult = 0

	--SAMPLE ITEM 10k GD
	--if(@in_ItemId = 301107) begin
	--	update UsersData set GameDollars=GameDollars+10000 where CustomerID=@in_CustomerID
	--	return
	--end

	-- clan items. NOTE: no item adding
	if(@in_ItemId >= 301151 and @in_ItemId <= 301157) begin
		return
	end

	-- normal item
	exec FN_AddItemToUser @in_CustomerID, @in_ItemId, @in_BuyDays
	set @out_FNResult = 0

END
GO


-- ----------------------------
-- procedure structure for WZ_BuyItemFN_GetPrice
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_BuyItemFN_GetPrice]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_BuyItemFN_GetPrice]
GO

CREATE PROCEDURE [dbo].[WZ_BuyItemFN_GetPrice]
    @out_FNResult int out,
	@in_ItemId int,
	@in_BuyDays int,
	@in_Currency varchar(32),
	@o_FinalPrice int out
AS
BEGIN
	SET NOCOUNT ON;

--
-- get prices from table based on itemID
--
	declare @Price1 int = 0
	declare @Price7 int = 0
	declare @Price30 int = 0
	declare @PriceP int = 0
	declare @GPrice1 int = 0
	declare @GPrice7 int = 0
	declare @GPrice30 int = 0
	declare @GPriceP int = 0
	declare @IsEnabled int = 1

	if(@in_ItemId >= 20000 and @in_ItemId < 99999)
		SELECT
		   @Price1=Price1, @Price7=Price7, @Price30=Price30, @PriceP=PriceP,
		   @GPrice1=GPrice1, @GPrice7=GPrice7, @GPrice30=GPrice30, @GPriceP=GPriceP
		FROM Items_Gear where ItemID=@in_ItemID
	else
	if(@in_ItemId >= 100000 and @in_ItemId < 190000)
		SELECT
		   @Price1=Price1, @Price7=Price7, @Price30=Price30, @PriceP=PriceP,
		   @GPrice1=GPrice1, @GPrice7=GPrice7, @GPrice30=GPrice30, @GPriceP=GPriceP
		FROM Items_Weapons where ItemID=@in_ItemID
	else
	if(@in_ItemId >= 300000 and @in_ItemId < 390000)
		SELECT
		   @Price1=Price1, @Price7=Price7, @Price30=Price30, @PriceP=PriceP,
		   @GPrice1=GPrice1, @GPrice7=GPrice7, @GPrice30=GPrice30, @GPriceP=GPriceP
		FROM Items_Generic where ItemID=@in_ItemID
	else
	if(@in_ItemId >= 400000 and @in_ItemId < 490000)
		SELECT
		   @Price1=Price1, @Price7=Price7, @Price30=Price30, @PriceP=PriceP,
		   @GPrice1=GPrice1, @GPrice7=GPrice7, @GPrice30=GPrice30, @GPriceP=GPriceP
		FROM Items_Attachments where ItemID=@in_ItemID
	else
	begin
		set @out_FNResult = 1
		return
	end
	if (@@RowCount = 0) begin
		set @out_FNResult = 2
		return
	end

	     if(@in_Currency = 'GP' and @in_BuyDays = 1)    set @o_FinalPrice = @Price1
	else if(@in_Currency = 'GP' and @in_BuyDays = 7)    set @o_FinalPrice = @Price7
	else if(@in_Currency = 'GP' and @in_BuyDays = 30)   set @o_FinalPrice = @Price30
	else if(@in_Currency = 'GP' and @in_BuyDays = 2000) set @o_FinalPrice = @PriceP
	else if(@in_Currency = 'GD' and @in_BuyDays = 1)    set @o_FinalPrice = @GPrice1
	else if(@in_Currency = 'GD' and @in_BuyDays = 7)    set @o_FinalPrice = @GPrice7
	else if(@in_Currency = 'GD' and @in_BuyDays = 30)   set @o_FinalPrice = @GPrice30
	else if(@in_Currency = 'GD' and @in_BuyDays = 2000) set @o_FinalPrice = @GPriceP
	else begin
		set @out_FNResult = 3
		return
	end

	-- check if listed
	if(@o_FinalPrice <= 0 or @IsEnabled = 0) begin
		set @out_FNResult = 4
		return
	end

	set @out_FNResult = 0
END
GO


-- ----------------------------
-- procedure structure for WZ_Char_SRV_SetAttachments
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_Char_SRV_SetAttachments]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_Char_SRV_SetAttachments]
GO

CREATE PROCEDURE [dbo].[WZ_Char_SRV_SetAttachments]
	@in_CustomerID int,
	@in_CharID int,
	@in_Attm1 varchar(256),
	@in_Attm2 varchar(256)
AS
BEGIN
	SET NOCOUNT ON;

	--
	-- this function should be called only by server, so we skip all validations
	--

	-- update attachments
	UPDATE UsersChars SET Attachment1=@in_Attm1, Attachment2=@in_Attm2 where CharID=@in_CharID

	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_Char_SRV_SetStatus
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_Char_SRV_SetStatus]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_Char_SRV_SetStatus]
GO

CREATE PROCEDURE [dbo].[WZ_Char_SRV_SetStatus]
	@in_CustomerID int,
	@in_CharID int,
	@in_Alive int,
	@in_GamePos varchar(256),
	@in_GameFlags int,
	@in_Health float,
	@in_Hunger float,
	@in_Thirst float,
	@in_Toxic float,
	@in_TimePlayed int,
	@in_XP int,
	@in_Reputation int,
	@in_GameDollars int,
	@in_Stat00 int,
	@in_Stat01 int,
	@in_Stat02 int,
	@in_Stat03 int,
	@in_Stat04 int,
	@in_Stat05 int
AS
BEGIN
	SET NOCOUNT ON;

	--
	-- this function should be called only by server, so we skip all validations
	--

	-- record last game update
	update UsersData set GameDollars=@in_GameDollars, lastgamedate=GETDATE() where CustomerID=@in_CustomerID

	-- update basic character data
	update UsersChars set
		GamePos=@in_GamePos,
		GameFlags=@in_GameFlags,
		Alive=@in_Alive,
		Health=@in_Health,
		Food=@in_Hunger,
		Water=@in_Thirst,
		Toxic=@in_Toxic,
		TimePlayed=@in_TimePlayed,
		LastUpdateDate=GETDATE(),
		XP=@in_XP,
		Reputation=@in_Reputation,
		Stat00=@in_Stat00,
		Stat01=@in_Stat01,
		Stat02=@in_Stat02,
		Stat03=@in_Stat03,
		Stat04=@in_Stat04,
		Stat05=@in_Stat05
	where CharID=@in_CharID

	if(@in_Alive = 0) begin
		update UsersChars set DeathUtcTime=GETUTCDATE() where CharID=@in_CharID
		-- set default backpack on death
		update UsersChars set BackpackID=21712, BackpackSize=16 where CharID=@in_CharID
		-- delete stuff from backpack
		delete from UsersInventory where CustomerID=@in_CustomerID and CharID=@in_CharID
	end

	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_CharRevive
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_CharRevive]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_CharRevive]
GO

CREATE PROCEDURE [dbo].[WZ_CharRevive]
	@in_CustomerID int,
	@in_CharID int
AS
BEGIN
	SET NOCOUNT ON;

	-- validate CharID/CustomerID pair
	declare @CustomerID int = 0
	select @CustomerID=CustomerID from UsersChars where CharID=@in_CharID
	if(@@ROWCOUNT = 0 or @CustomerID <> @in_CustomerID) begin
		select 6 as ResultCode, 'bad charid' as ResultMsg
		return
	end

	-- get developer flag
	declare @IsDeveloper int = 0
	select @IsDeveloper=IsDeveloper from UsersData where CustomerID=@in_CustomerID

	-- note that revive timer is 1hrs, change in WZ_GetAccountInfo1 as well
	declare @SecToRevive int
	declare @Alive int = 0
	select
		@SecToRevive=DATEDIFF(second, GETUTCDATE(), DATEADD(hour, 1, DeathUtcTime)),
		@Alive=Alive
	from UsersChars where CharID=@in_CharID

	-- prevent fast teleporting if we're not dead
	if(@Alive <> 0) begin
		select 6 as ResultCode, 'character is not dead' as ResultMsg
		return
	end

	-- do not allow early revive, give 2min grace
	if(@SecToRevive > 120 and @IsDeveloper = 0) begin
		select 6 as ResultCode, 'too early' as ResultMsg
		return
	end

	-- revive
	update UsersChars set
		Alive=2,
		Health=100,
		Food=0,
		Water=0,
		Toxic=0,
		GameFlags=1
	where CharID=@in_CharID

	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanAddClanMembers
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanAddClanMembers]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanAddClanMembers]
GO

CREATE PROCEDURE [dbo].[WZ_ClanAddClanMembers]
	@in_CharID int,
	@in_ItemID int
AS
BEGIN
	SET NOCOUNT ON;

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

	-- add members value is in permanent GD price
	declare @GPriceP int = 0
	select @GPriceP=GPriceP from Items_Generic where ItemID=@in_ItemID
	if(@GPriceP = 0) begin
		select 6 as ResultCode, 'no price1' as ResultMsg
		return
	end

	-- update clan
	update ClanData set MaxClanMembers=(MaxClanMembers+@GPriceP) where ClanID=@ClanID

	-- generate clan event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Var2,
		Text1
	) values (
		@ClanID,
		GETDATE(),
		13, -- ClanEvent_AddMaxMembers
		99, -- Visible to all
		@in_CharID,
		@GPriceP,
		@Gamertag
	)

	-- success
	select 0 as ResultCode

END
GO


-- ----------------------------
-- procedure structure for WZ_ClanApplyAnswer
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanApplyAnswer]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanApplyAnswer]
GO

CREATE PROCEDURE [dbo].[WZ_ClanApplyAnswer]
	@in_CharID int,
	@in_ClanApplicationID int,
	@in_Answer int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

	-- only leader and officers can answer application
	if(@ClanRank > 1) begin
		select 23 as ResultCode, 'no permission' as ResultMsg
		return
	end

	-- check if we have enough slots in clan
	declare @MaxClanMembers int
	declare @NumClanMembers int
	select @MaxClanMembers=MaxClanMembers, @NumClanMembers=NumClanMembers from ClanData where ClanID=@ClanID
	if(@NumClanMembers >= @MaxClanMembers) begin
		select 20 as 'ResultCode', 'not enough slots' as ResultMsg
		return
	end

-- check application

	declare @AppClanID int = 0
	declare @AppCharID int
	select @AppClanID=ClanID, @AppCharID=CharID	from ClanApplications where ClanApplicationID=@in_ClanApplicationID
	if(@AppClanID <> @ClanID) begin
		select 6 as ResultCode, 'bad application id' as ResultMsg
		return
	end

	-- mark that application as processed
	update ClanApplications set IsProcessed=1 where ClanApplicationID=@in_ClanApplicationID

	-- make sure that this guy isn't joined other clan somehow (race condition)
	declare @AppGamertag nvarchar(64)
	select @AppClanID=ClanID, @AppGamertag=Gamertag from UsersChars where CharID=@AppCharID
	if(@AppClanID <> 0) begin
		select 21 as ResultCode, 'applicant already joined clan' as ResultMsg
		return
	end

	if(@in_Answer = 0)
	begin
		-- declined clan joining
		-- TODO: send message to player about denial

		select 0 as ResultCode
		return
	end

	-- accept application, join player to clan
	update ClanData set NumClanMembers=(NumClanMembers + 1) where ClanID=@ClanID
	update UsersChars set ClanID=@ClanID, ClanRank=99 where CharID=@AppCharID

	-- clear all other applications
	delete from ClanApplications where CharID=@AppCharID

-- generate clan event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Text1
	) values (
		@ClanID,
		GETDATE(),
		4, -- CLANEvent_Join
		99, -- Visible to all
		@AppCharID,
		@AppGamertag
	)

	select 0 as ResultCode
	return

END
GO


-- ----------------------------
-- procedure structure for WZ_ClanApplyGetList
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanApplyGetList]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanApplyGetList]
GO

CREATE PROCEDURE [dbo].[WZ_ClanApplyGetList]
	@in_CharID int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

-- give list of applyers

	-- only leader and officers can view application list
	if(@ClanRank > 1) begin
		select 6 as ResultCode, 'no permission' as ResultMsg
		return
	end

	-- success
	select 0 as ResultCode

	select
		a.ClanApplicationID,
		a.ApplicationText,
		DATEDIFF(mi, GETDATE(), a.ExpireTime) as MinutesLeft,
		c.*
	from ClanApplications a
	join UsersChars c on (c.CharID=a.CharID)
	where a.ClanID=@ClanID and GETDATE()<ExpireTime and IsProcessed=0

	return

END
GO


-- ----------------------------
-- procedure structure for WZ_ClanApplyToJoin
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanApplyToJoin]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanApplyToJoin]
GO

CREATE PROCEDURE [dbo].[WZ_ClanApplyToJoin]
	@in_CharID int,
	@in_ClanID int,
	@in_ApplicationText nvarchar(500)
AS
BEGIN
	SET NOCOUNT ON;

	declare @APPLY_EXPIRE_TIME_HOURS int = 72
	declare @MAX_PENDING_APPS int = 5	-- can be maximum 5 pending invitations

-- sanity checks

	-- player must be without clan
	declare @PlayerClanID int = 0
	select @PlayerClanID=ClanID from UsersChars where CharID=@in_CharID
	if(@PlayerClanID > 0) begin
		select 6 as ResultCode, 'already in clan' as ResultMsg
		return
	end

	-- make sure clan exists
	if not exists (select ClanID from ClanData where ClanID=@in_ClanID) begin
		select 6 as ResultCode, 'no clanid' as ResultMsg
		return
	end

	-- see if we already have pending invidation
	declare @AppExpireTime datetime
	select @AppExpireTime=ExpireTime from ClanApplications where ClanID=@in_ClanID and CharID=@in_CharID and GETDATE()<ExpireTime
	if(@@ROWCOUNT > 0) begin
		select 24 as ResultCode, 'pending application' as ResultMsg
		return
	end

	-- see if we already have too much applications
	declare @AppTotalCounts int = 0
	select @AppTotalCounts=COUNT(*) from ClanApplications where CharID=@in_CharID and GETDATE()<ExpireTime
	if(@AppTotalCounts >= @MAX_PENDING_APPS) begin
		select 25 as ResultCode, 'too many applications' as ResultMsg
		return
	end

-- send application

	insert into ClanApplications (
		ClanID,
		CharID,
		ExpireTime,
		ApplicationText,
		IsProcessed
	) values (
		@in_ClanID,
		@in_CharID,
		DATEADD(hour, @APPLY_EXPIRE_TIME_HOURS, GETDATE()),
		@in_ApplicationText,
		0
	)

	-- success
	select 0 as ResultCode
	return

END
GO


-- ----------------------------
-- procedure structure for WZ_ClanCreate
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanCreate]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanCreate]
GO

CREATE PROCEDURE [dbo].[WZ_ClanCreate]
	@in_CustomerID int,
	@in_CharID int,
	@in_ClanName nvarchar(64),
	@in_ClanNameColor int,
	@in_ClanTag nvarchar(4),
	@in_ClanTagColor int,
	@in_ClanEmblemID int,
	@in_ClanEmblemColor int
AS
BEGIN
	SET NOCOUNT ON;

	declare @DEFAULT_CLAN_SIZE int = 15

	-- sanity check
	declare @ClanID int = 0
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID > 0) begin
		select 6 as 'ResultCode', 'already have clan' as 'ResultMsg'
		return
	end

	-- check for 20 hour play time
	declare @TimePlayedHours int = 0
	select @TimePlayedHours=(TimePlayed/3600) from UsersData where CustomerID=@in_CustomerID
	if(@TimePlayedHours < 20) begin
		select 29 as 'ResultCode', 'Clan can only be created after 20 hours total play time' as 'ResultMsg'
		return
	end

	-- create clan!
	insert into ClanData (
		ClanName, ClanNameColor,
		ClanTag, ClanTagColor,
		ClanEmblemID, ClanEmblemColor,
		ClanXP,	ClanLevel, ClanGP,
		OwnerCustomerID, OwnerCharID,
		MaxClanMembers, NumClanMembers,
		ClanCreateDate
	) values (
		@in_ClanName, @in_ClanNameColor,
		@in_ClanTag, @in_ClanTagColor,
		@in_ClanEmblemID, @in_ClanEmblemColor,
		0,	0,	0,
		@in_CustomerID, @in_CharID,
		@DEFAULT_CLAN_SIZE,	1,
		GETDATE()
	)

	-- get new clanID
	select @ClanID=ClanID from ClanData where OwnerCharID=@in_CharID
	if(@@ROWCOUNT = 0) begin
		select 6 as 'ResultCode', 'clan creation failed!' as 'ResultMsg'
		return
	end

	-- update owner clan data
	update UsersChars set ClanID=@ClanID, ClanRank=0 where CharID=@in_CharID

	-- generate clan event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Text1
	) values (
		@ClanID,
		GETDATE(),
		1, -- CLANEVENT_Created
		99, -- Visible to all
		@in_CharID,
		@Gamertag
	)

	-- success
	select 0 as ResultCode

	select @ClanID as 'ClanID'
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanCreateCheckMoney
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanCreateCheckMoney]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanCreateCheckMoney]
GO

CREATE PROCEDURE [dbo].[WZ_ClanCreateCheckMoney]
	@in_CustomerID int
AS
BEGIN
	SET NOCOUNT ON;

	-- this call is always valid
	select 0 as ResultCode

	-- doesn't need money yet
	select 0 as NeedMoney

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanCreateCheckParams
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanCreateCheckParams]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanCreateCheckParams]
GO

CREATE PROCEDURE [dbo].[WZ_ClanCreateCheckParams]
	@in_CharID int,
	@in_ClanName nvarchar(64),
	@in_ClanTag nvarchar(4)
AS
BEGIN
	SET NOCOUNT ON;

	-- user can't create more that one clan
	declare @ClanID int = 0
	select @ClanID=ClanID from UsersChars where CharID=@in_CharID
	if(@ClanID > 0) begin
		select 6 as ResultCode, 'already have clan' as ResultMsg
		return
	end

	-- check that name/tag is unique
	if(exists(select * from ClanData where ClanName=@in_ClanName)) begin
		select 27 as ResultCode, 'clan name' as ResultMsg
		return
	end
	if(exists(select * from ClanData where ClanTag=@in_ClanTag)) begin
		select 28 as ResultCode, 'clan tag' as ResultMsg
		return
	end

	select 0 as ResultCode
	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanDonateToClanGP
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanDonateToClanGP]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanDonateToClanGP]
GO

CREATE PROCEDURE [dbo].[WZ_ClanDonateToClanGP]
	@in_CustomerID int,
	@in_CharID int,
	@in_GP int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks
	declare @ClanID int
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'not in clan' as ResultMsg
		return
	end

	declare @GamePoints int = 0
	select @GamePoints=GamePoints from UsersData where CustomerID=@in_CustomerID

	if(@in_GP < 0) begin
		select 6 as ResultCode, 'sneaky bastard...' as ResultMsg
		return
	end
	if(@in_GP > @GamePoints) begin
		select 6 as ResultCode, 'not enough GP' as ResultMsg
		return
	end

-- donating

	-- substract GP
	declare @AlterGP int = -@in_GP
	exec FN_AlterUserGP @in_CustomerID, @AlterGP, 'toclan'
	update UsersChars set ClanContributedGP=(ClanContributedGP+@in_GP) where CharID=@in_CharID
	-- and record that
	INSERT INTO FinancialTransactions
		VALUES (@in_CustomerID, 'CLAN_GPToClan', 4000, GETDATE(),
				@in_GP, '1', 'APPROVED', @ClanID)

	-- add clan gp
	update ClanData set ClanGP=(ClanGP+@in_GP) where ClanID=@ClanID

-- generate clan event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Var3,
		Text1
	) values (
		@ClanID,
		GETDATE(),
		10, -- CLANEvent_DonateToClanGP
		99, -- Visible to all
		@in_CharID,
		@in_GP,
		@Gamertag
	)

	-- success
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanDonateToMemberGP
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanDonateToMemberGP]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanDonateToMemberGP]
GO

CREATE PROCEDURE [dbo].[WZ_ClanDonateToMemberGP]
	@in_CharID int,
	@in_GP int,
	@in_MemberID int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks

	-- clan id valudation of caller
	declare @ClanID int
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'not in clan' as ResultMsg
		return
	end

	-- clan id validation of member
	declare @MemberClanID int = 0
	declare @MemberGamerTag nvarchar(64)
	select @MemberClanID=ClanID, @MemberGamerTag=GamerTag from UsersChars where CharID=@in_MemberID
	if(@MemberClanID <> @ClanID) begin
		select 6 as ResultCode, 'member in wrong clan' as ResultMsg
		return
	end

-- donating
	if(@ClanRank > 0) begin
		select 23 as ResultCode, 'no permission' as ResultMsg
		return
	end

	declare @ClanGP int = 0
	select @ClanGP=ClanGP from ClanData where ClanID=@ClanID
	if(@in_GP < 0) begin
		select 6 as ResultCode, 'sneaky bastard...' as ResultMsg
		return
	end
	if(@in_GP > @ClanGP) begin
		select 6 as ResultCode, 'not enough GP in clan' as ResultMsg
		return
	end

	-- substract GP from clan
	update ClanData set ClanGP=(ClanGP-@in_GP) where ClanID=@ClanID

	-- add member gp
	exec FN_AlterUserGP @in_MemberID, @in_GP, 'fromclan'
	-- and record that
	INSERT INTO FinancialTransactions
		VALUES (@in_MemberID, 'CLAN_GPToMember', 4001, GETDATE(),
				@in_GP, '1', 'APPROVED', @ClanID)

-- generate clan event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Var2,
		Var3,
		Text1,
		Text2
	) values (
		@ClanID,
		GETDATE(),
		11, -- CLANEvent_DonateToMemberGP
		1, -- Visible to officers
		@in_CharID,
		@in_MemberID,
		@in_GP,
		@Gamertag,
		@MemberGamertag
	)

-- TODO: send message to player about donate

	-- success
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanFN_DeleteClan
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanFN_DeleteClan]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanFN_DeleteClan]
GO

CREATE PROCEDURE [dbo].[WZ_ClanFN_DeleteClan]
	@in_ClanID int
AS
BEGIN
	SET NOCOUNT ON;

	delete from ClanData where ClanID=@in_ClanID
	delete from ClanApplications where ClanID=@in_ClanID
	delete from ClanInvites where ClanID=@in_ClanID

	update UsersChars set ClanID=0 where ClanID=@in_ClanID

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanGetEvents
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanGetEvents]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanGetEvents]
GO

CREATE PROCEDURE [dbo].[WZ_ClanGetEvents]
	@in_CharID int,
	@in_Days int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

-- report clan log
	select 0 as ResultCode

	declare @MinDate datetime = DATEADD(day, -@in_Days, GETDATE())
	select * from ClanEvents where ClanID=@ClanID and EventDate>=@MinDate and @ClanRank <= EventRank order by EventDate asc

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanGetInfo
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanGetInfo]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanGetInfo]
GO

CREATE PROCEDURE [dbo].[WZ_ClanGetInfo]
	@in_ClanID int,
	@in_GetMembers int
AS
BEGIN
	SET NOCOUNT ON;

	-- success
	select 0 as ResultCode

	-- and report clan data
	if(@in_ClanID > 0)
	begin
		-- specific clan
		select *, c.gamertag from ClanData
			left join UsersChars c on c.CharID=ClanData.OwnerCharID
			where ClanData.ClanID=@in_ClanID
	end
	else
	begin
		-- all clans
		select *, c.gamertag from ClanData
			left join UsersChars c on c.CharID=ClanData.OwnerCharID
	end

	-- if need to report members
	if(@in_ClanID > 0 and @in_GetMembers > 0) begin
		select UsersChars.* from UsersChars	where ClanID=@in_ClanID
	end

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanGetPlayerData
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanGetPlayerData]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanGetPlayerData]
GO

CREATE PROCEDURE [dbo].[WZ_ClanGetPlayerData]
	@in_CharID int
AS
BEGIN
	SET NOCOUNT ON;

	-- success
	select 0 as ResultCode

	-- report player clan id and current clan info
	select c.ClanID, c.ClanRank, d.*
		from UsersChars c
		left join ClanData d on d.ClanID=c.ClanID
		where CharID=@in_CharID

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanInviteAnswer
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanInviteAnswer]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanInviteAnswer]
GO

CREATE PROCEDURE [dbo].[WZ_ClanInviteAnswer]
	@in_CharID int,
	@in_ClanInviteID int,
	@in_Answer int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks

	-- must be free to join clan
	declare @ClanID int = 0
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID <> 0) begin
		select 6 as ResultCode, 'already in clan' as ResultMsg
		return
	end

	-- have valid invitation id (get actual ClanID here)
	declare @InvCharID int
	select @ClanID=ClanID, @InvCharID=CharID from ClanInvites where ClanInviteID=@in_ClanInviteID
	if(@@ROWCOUNT = 0) begin
		select 6 as ResultCode, 'bad inviteid #1' as ResultMsg
		return
	end
	if(@InvCharID <> @in_CharID) begin
		select 6 as ResultCode, 'bad inviteid #2' as ResultMsg
		return
	end

-- invite

	-- delete invite anyway
	delete from ClanInvites where ClanInviteID=@in_ClanInviteID

	-- check if invite is denied
	if(@in_Answer = 0) begin
		select 0 as ResultCode
		select @ClanID as ClanID
		return
	end

	-- check if we have enough slots in clan
	declare @MaxClanMembers int
	declare @NumClanMembers int
	select @MaxClanMembers=MaxClanMembers, @NumClanMembers=NumClanMembers from ClanData where ClanID=@ClanID
	if(@NumClanMembers >= @MaxClanMembers) begin
		select 20 as 'ResultCode', 'not enough slots' as ResultMsg
		return
	end

	-- join the clan!
	update ClanData set NumClanMembers=(NumClanMembers + 1) where ClanID=@ClanID
	update UsersChars set ClanID=@ClanID, ClanRank=99 where CharID=@in_CharID

-- generate clan event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Text1
	) values (
		@ClanID,
		GETDATE(),
		4, -- CLANEvent_Join
		99, -- Visible to officers
		@in_CharID,
		@Gamertag
	)

	-- success
	select 0 as ResultCode
	select @ClanID as ClanID
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanInviteGetInvitesForPlayer
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanInviteGetInvitesForPlayer]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanInviteGetInvitesForPlayer]
GO

CREATE PROCEDURE [dbo].[WZ_ClanInviteGetInvitesForPlayer]
	@in_CharID int
AS
BEGIN
	SET NOCOUNT ON;

-- report all pending invites

	select 0 as ResultCode

	select
		i.ClanInviteID,
		c.Gamertag,
		d.*
	from ClanInvites i
	left join UsersChars c on (c.CharID=i.InviterCharID)
	join ClanData d on (d.ClanID=i.ClanID)
	where i.CharID=@in_CharID and GETDATE()<ExpireTime

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanInviteReportAll
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanInviteReportAll]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanInviteReportAll]
GO

CREATE PROCEDURE [dbo].[WZ_ClanInviteReportAll]
	@in_CharID int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

-- validate that we can invite

	-- only leader and officers can invite
	if(@ClanRank > 1) begin
		select 6 as ResultCode, 'no permission' as ResultMsg
		return
	end

-- report all pending invites

	-- success
	select 0 as ResultCode

	select
		i.ClanInviteID,
		c.Gamertag,
		DATEDIFF(mi, GETDATE(), i.ExpireTime) as MinutesLeft
	from ClanInvites i
	join UsersChars c on (c.CharID=i.CharID)
	where i.ClanID=@ClanID and GETDATE()<ExpireTime

	return
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanInviteSendToPlayer
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanInviteSendToPlayer]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanInviteSendToPlayer]
GO

CREATE PROCEDURE [dbo].[WZ_ClanInviteSendToPlayer]
	@in_CharID int,
	@in_InvGamertag nvarchar(64)
AS
BEGIN
	SET NOCOUNT ON;

	declare @INVITE_EXPIRE_TIME_HOURS int = 72

-- sanity checks

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'not in clan' as ResultMsg
		return
	end

-- validate that we can invite

	-- only leader and officers can invite
	if(@ClanRank > 1) begin
		select 23 as ResultCode, 'no permission' as ResultMsg
		return
	end

	-- check if we have enough slots in clan
	declare @MaxClanMembers int
	declare @NumClanMembers int
	select @MaxClanMembers=MaxClanMembers, @NumClanMembers=NumClanMembers from ClanData where ClanID=@ClanID

	declare @PendingInvites int = 0
	--DISABLED FOR NOW: select @PendingInvites=COUNT(*) from ClanInvites where ClanID=@ClanID and GETDATE()<ExpireTime
	if((@NumClanMembers + @PendingInvites) >= @MaxClanMembers) begin
		select 20 as 'ResultCode', 'not enough slots' as ResultMsg
		return
	end

	-- check if user exists
	declare @InvCharID int
	declare @InvClanID int
	select @InvCharID=CharID, @InvClanID=ClanID from UsersChars where Gamertag=@in_InvGamertag
	if(@@ROWCOUNT = 0) begin
		select 22 as ResultCode, 'no such gamertag' as ResultMsg
		return
	end
	-- and have no clan
	if(@InvClanID <> 0) begin
		select 21 as ResultCode, 'already in clan' as ResultMsg
		return
	end

	-- check if we have pending invite
	if(exists(select * from ClanInvites where ClanID=@ClanID and CharID=@InvCharID and GETDATE()<ExpireTime)) begin
		select 24 as ResultCode, 'already invited' as ResultMsg
		return
	end

-- invite
	insert into ClanInvites (
		ClanID,
		InviterCharID,
		CharID,
		ExpireTime
	) values (
		@ClanID,
		@in_CharID,
		@InvCharID,
		DATEADD(hour, @INVITE_EXPIRE_TIME_HOURS, GETDATE())
	)

	-- success
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanKickMember
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanKickMember]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanKickMember]
GO

CREATE PROCEDURE [dbo].[WZ_ClanKickMember]
	@in_CharID int,
	@in_MemberID int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks
	if(@in_CharID = @in_MemberID) begin
		select 6 as 'ResultCode', 'cant kick himselft' as 'ResultMsg'
		return
	end

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

	-- clan id validation of member
	declare @MemberClanID int = 0
	declare @MemberGamerTag nvarchar(64)
	declare @MemberClanRank int
	select @MemberClanID=ClanID, @MemberClanRank=ClanRank, @MemberGamerTag=GamerTag from UsersChars where CharID=@in_MemberID
	if(@MemberClanID <> @ClanID) begin
		select 6 as ResultCode, 'member in wrong clan' as ResultMsg
		return
	end

-- validate that we can kick

	-- only leader and officers can kick
	if(@ClanRank > 1) begin
		select 23 as ResultCode, 'no permission' as ResultMsg
		return
	end

	-- cant kick higher rank
	if(@ClanRank > 0 and @ClanRank >= @MemberClanRank) begin
		select 6 as ResultCode, 'cant kick highter rank' as ResultMsg
		return
	end

-- update clan info and kick player
	update ClanData set NumClanMembers=(NumClanMembers-1) where ClanID=@ClanID
	update UsersChars set ClanID=0, ClanContributedGP=0, ClanContributedXP=0 where CharID=@in_MemberID

-- generate clan event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Var2,
		Text1,
		Text2
	) values (
		@ClanID,
		GETDATE(),
		6, -- CLANEvent_Kick
		99, -- Visible to all
		@in_CharID,
		@in_MemberID,
		@Gamertag,
		@MemberGamertag
	)

	-- TODO: send message to player about kick

	-- success
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanLeave
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanLeave]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanLeave]
GO

CREATE PROCEDURE [dbo].[WZ_ClanLeave]
	@in_CharID int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks
	declare @ClanID int
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

-- leader is leaving clan
	if(@ClanRank = 0)
	begin
		declare @NumClanMembers int
		select @NumClanMembers=COUNT(*) from UsersChars where ClanID=@ClanID
		if(@NumClanMembers > 1) begin
			select 6 as ResultCode, 'owner cant leave - there is people left in clan' as ResultMsg
			return
		end

		-- generate clan event
		insert into ClanEvents (
			ClanID,
			EventDate,
			EventType,
			EventRank,
			Var1,
			Text1
		) values (
			@ClanID,
			GETDATE(),
			99, -- CLANEvent_Disband
			99, -- Visible to all
			@in_CharID,
			@Gamertag
		)

		-- and delete clan
		exec WZ_ClanFN_DeleteClan @ClanID

		select 0 as ResultCode
		return
	end

	-- actual leave
	update UsersChars set ClanID=0, ClanContributedGP=0, ClanContributedXP=0 where CharID=@in_CharID
	update ClanData set NumClanMembers=(NumClanMembers - 1) where ClanID=@ClanID

-- generate clan event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Text1
	) values (
		@ClanID,
		GETDATE(),
		5, -- CLANEvent_Left
		99, -- Visible to all
		@in_CharID,
		@Gamertag
	)

	-- success
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanSetLore
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanSetLore]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanSetLore]
GO

CREATE PROCEDURE [dbo].[WZ_ClanSetLore]
	@in_CharID int,
	@in_Lore nvarchar(512)
AS
BEGIN
	SET NOCOUNT ON;

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int = 99
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

	-- only leader and officers can change lore
	if(@ClanRank > 1) begin
		select 23 as ResultCode, 'no permission' as ResultMsg
		return
	end

	update ClanData set ClanLore=@in_Lore where ClanID=@ClanID

	-- success
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_ClanSetMemberRank
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ClanSetMemberRank]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ClanSetMemberRank]
GO

CREATE PROCEDURE [dbo].[WZ_ClanSetMemberRank]
	@in_CharID int,
	@in_MemberID int,
	@in_Rank int
AS
BEGIN
	SET NOCOUNT ON;

-- sanity checks
	if(@in_CharID = @in_MemberID) begin
		select 6 as 'ResultCode', 'can not set own rank' as 'ResultMsg'
		return
	end

	-- clan id valudation of caller
	declare @ClanID int = 0
	declare @ClanRank int
	declare @Gamertag nvarchar(64)
	select @ClanID=ClanID, @ClanRank=ClanRank, @Gamertag=Gamertag from UsersChars where CharID=@in_CharID
	if(@ClanID = 0) begin
		select 6 as ResultCode, 'no clan' as ResultMsg
		return
	end

	-- clan id validation of member
	declare @MemberClanID int = 0
	declare @MemberGamerTag nvarchar(64)
	declare @MemberClanRank int
	declare @MemberCustomerID int
	select @MemberClanID=ClanID, @MemberClanRank=ClanRank, @MemberGamerTag=GamerTag, @MemberCustomerID=CustomerID from UsersChars where CharID=@in_MemberID
	if(@MemberClanID <> @ClanID) begin
		select 6 as ResultCode, 'member in wrong clan' as ResultMsg
		return
	end

-- validate that we can change rank

	-- only leader and officers can change ranks
	if(@ClanRank > 1) begin
		select 23 as ResultCode, 'no permission' as ResultMsg
		return
	end

	-- cant change higher rank
	if(@ClanRank > 0 and @ClanRank >= @MemberClanRank) begin
		select 6 as ResultCode, 'cant change highter rank' as ResultMsg
		return
	end

	if(@ClanRank > 0 and @ClanRank >= @in_Rank) begin
		select 6 as ResultCode, 'cant set same rank' as ResultMsg
		return
	end

-- code for changing clan ownership, owner becomes officer
	if(@ClanRank = 0 and @in_Rank = 0) begin
		update UsersChars set ClanRank=1 where CharID=@in_CharID
		update ClanData set OwnerCharID=@in_MemberID, OwnerCustomerID=@MemberCustomerID where ClanID=@ClanID
	end

-- update target member
	update UsersChars set ClanRank=@in_Rank where CharID=@in_MemberID

-- generate clan set rank event
	insert into ClanEvents (
		ClanID,
		EventDate,
		EventType,
		EventRank,
		Var1,
		Var2,
		Var3,
		Text1,
		Text2
	) values (
		@ClanID,
		GETDATE(),
		3, -- CLANEVENT_SetRank
		99, -- Visible to all
		@in_CharID,
		@in_MemberID,
		@in_Rank,
		@Gamertag,
		@MemberGamertag
	)

	-- TODO: send message to player about rank change

	-- success
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_DB_GenerateDailyLeaderboard
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_DB_GenerateDailyLeaderboard]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_DB_GenerateDailyLeaderboard]
GO

CREATE PROCEDURE [dbo].[WZ_DB_GenerateDailyLeaderboard]
AS
BEGIN
	SET NOCOUNT ON;

	/*
	The Query Processor estimates that implementing the following index could improve the query cost by 13.035%.
	NOTE: using index IX_DBG_UserRoundResults_LBIdx1 on DBG_UserRoundResults
	*/

	declare @CurDay date = GETDATE() -- date is truncated to day only, no time

	-- reset table with zero seed
	delete from Leaderboard1
	DBCC CHECKIDENT (Leaderboard1, RESEED, 0)

	-- insert all ordered by honorpoints
	insert into Leaderboard1 (
			CustomerID, gamertag, HonorPoints,
			Kills, Deaths, Wins, Losses, ShotsFired, ShotsHit,
			TimePlayed,
			Rank,
			HavePremium)
		select
			urr.CustomerID,
			l.Gamertag,
			sum(urr.HonorPoints),
			sum(Kills), sum(Deaths), sum(Wins), sum(Losses), sum(ShotsFired), sum(ShotsHits),
			sum(TimePlayed),
			-- not need rank now
			0, --(select top(1) rank from DataRankPoints where LoginID.HonorPoints<DataRankPoints.HonorPoints order by HonorPoints asc),
			-- check if have premium
			(case when exists (select * from Inventory where ItemID=301004 and Inventory.CustomerID=urr.CustomerID and LeasedUntil>GETDATE())
				then 1
				else 0
			end)
		from DBG_UserRoundResults urr
		join LoginID l on (l.CustomerID=urr.CustomerID)
		where GameReportTime>=@CurDay and l.AccountStatus=101
		group by urr.CustomerID, l.Gamertag
		order by sum(urr.HonorPoints) desc
END
GO


-- ----------------------------
-- procedure structure for WZ_DB_GenerateLeaderboard
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_DB_GenerateLeaderboard]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_DB_GenerateLeaderboard]
GO

CREATE PROCEDURE [dbo].[WZ_DB_GenerateLeaderboard]
AS
BEGIN
	SET NOCOUNT ON;

	-- reset table with zero seed
	delete from Leaderboard
	DBCC CHECKIDENT (Leaderboard, RESEED, 0)

	-- insert all ordered by honorpoints
	insert into Leaderboard (
			CustomerID, gamertag, HonorPoints,
			Kills, Deaths, Wins, Losses ,ShotsFired, ShotsHit,
			TimePlayed,
			Rank,
			HavePremium)
		select
			LoginID.CustomerID, gamertag, HonorPoints,
			Stats.Kills, Stats.Deaths, Stats.Wins, Stats.Losses, Stats.ShotsFired, Stats.ShotsHits,
			Stats.TimePlayed,
			-- get rank from table
			(select top(1) rank from DataRankPoints where LoginID.HonorPoints<DataRankPoints.HonorPoints order by HonorPoints asc),
			-- check if have premium
			(case when exists (select * from Inventory where ItemID=301004 and Inventory.CustomerID=LoginID.CustomerID and LeasedUntil>GETDATE())
				then 1
				else 0
			end)
		from LoginID
		join Stats on Stats.CustomerID=LoginID.CustomerID
		where AccountStatus=101
		order by LoginID.HonorPoints desc

END
GO


-- ----------------------------
-- procedure structure for WZ_DB_GenerateLeaderboard30
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_DB_GenerateLeaderboard30]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_DB_GenerateLeaderboard30]
GO

CREATE PROCEDURE [dbo].[WZ_DB_GenerateLeaderboard30]
AS
BEGIN
	SET NOCOUNT ON;

	/*
	The Query Processor estimates that implementing the following index could improve the query cost by 13.035%.
	NOTE: using index IX_DBG_UserRoundResults_LBIdx1 on DBG_UserRoundResults
	*/

	-- assemble start date of this month
	declare @StartDay date = '2000-01-01'
	set @StartDay = DATEADD(year, YEAR(GETDATE()) - 2000, @StartDay)
	set @StartDay = DATEADD(month, MONTH(GETDATE()) - 1, @StartDay)

	-- reset table with zero seed
	delete from Leaderboard30
	DBCC CHECKIDENT (Leaderboard30, RESEED, 0)

	-- insert all ordered by honorpoints
	insert into Leaderboard30 (
			CustomerID, gamertag, HonorPoints,
			Kills, Deaths, Wins, Losses, ShotsFired, ShotsHit,
			TimePlayed,
			Rank,
			HavePremium)
		select
			urr.CustomerID,
			l.Gamertag,
			sum(urr.HonorPoints),
			sum(Kills), sum(Deaths), sum(Wins), sum(Losses), sum(ShotsFired), sum(ShotsHits),
			sum(TimePlayed),
			-- not need rank now
			0, --(select top(1) rank from DataRankPoints where LoginID.HonorPoints<DataRankPoints.HonorPoints order by HonorPoints asc),
			-- check if have premium
			(case when exists (select * from Inventory where ItemID=301004 and Inventory.CustomerID=urr.CustomerID and LeasedUntil>GETDATE())
				then 1
				else 0
			end)
		from DBG_UserRoundResults urr
		join LoginID l on (l.CustomerID=urr.CustomerID)
		where GameReportTime>=@StartDay and l.AccountStatus=101
		group by urr.CustomerID, l.Gamertag
		order by sum(urr.HonorPoints) desc
END
GO


-- ----------------------------
-- procedure structure for WZ_DB_GenerateLeaderboard7
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_DB_GenerateLeaderboard7]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_DB_GenerateLeaderboard7]
GO

CREATE PROCEDURE [dbo].[WZ_DB_GenerateLeaderboard7]
AS
BEGIN
	SET NOCOUNT ON;

	/*
	The Query Processor estimates that implementing the following index could improve the query cost by 13.035%.
	NOTE: using index IX_DBG_UserRoundResults_LBIdx1 on DBG_UserRoundResults
	*/

	-- assemble start date of this week
	declare @StartDay date = GETDATE()
	set @StartDay = DATEADD(day, -(DATEPART(weekday, @StartDay) - 1), @StartDay)

	-- reset table with zero seed
	delete from Leaderboard7
	DBCC CHECKIDENT (Leaderboard7, RESEED, 0)

	-- insert all ordered by honorpoints
	insert into Leaderboard7 (
			CustomerID, gamertag, HonorPoints,
			Kills, Deaths, Wins, Losses, ShotsFired, ShotsHit,
			TimePlayed,
			Rank,
			HavePremium)
		select
			urr.CustomerID,
			l.Gamertag,
			sum(urr.HonorPoints),
			sum(Kills), sum(Deaths), sum(Wins), sum(Losses), sum(ShotsFired), sum(ShotsHits),
			sum(TimePlayed),
			-- not need rank now
			0, --(select top(1) rank from DataRankPoints where LoginID.HonorPoints<DataRankPoints.HonorPoints order by HonorPoints asc),
			-- check if have premium
			(case when exists (select * from Inventory where ItemID=301004 and Inventory.CustomerID=urr.CustomerID and LeasedUntil>GETDATE())
				then 1
				else 0
			end)
		from DBG_UserRoundResults urr
		join LoginID l on (l.CustomerID=urr.CustomerID)
		where GameReportTime>=@StartDay and l.AccountStatus=101
		group by urr.CustomerID, l.Gamertag
		order by sum(urr.HonorPoints) desc
END
GO


-- ----------------------------
-- procedure structure for WZ_GetAccountInfo1
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_GetAccountInfo1]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_GetAccountInfo1]
GO

CREATE PROCEDURE [dbo].[WZ_GetAccountInfo1]
	@in_CustomerID int,
	@in_CharID int
AS
BEGIN
	SET NOCOUNT ON;

	-- check if CustomerID is valid
	if not exists (SELECT CustomerID FROM UsersData WHERE CustomerID=@in_CustomerID)
	begin
		select 6 as ResultCode
		return;
	end

	if(@in_CharID > 0) begin
		update UsersData set lastjoineddate=GETDATE() where CustomerID=@in_CustomerID
	end

	select 0 as ResultCode

	SELECT
		UsersData.*,
		DATEDIFF(ss, lastgamedate, GETDATE()) as 'SecFromLastGame',
		ClanData.ClanTag, ClanData.ClanTagColor
	FROM UsersData
	left JOIN ClanData on (UsersData.ClanID = ClanData.ClanID)
	where UsersData.CustomerID=@in_CustomerID

--
-- report chars
--
	if(@in_CharID > 0)
	begin
		-- single character, version called from server
		select
			0 as 'SecToRevive',
			*
		from UsersChars where CustomerID=@in_CustomerID and CharID=@in_CharID
	end
	else
	begin
		-- note that revive timer is 1hrs, change in WZ_CharRevive as well
		select
			DATEDIFF(second, GETUTCDATE(), DATEADD(hour, 1, DeathUtcTime)) as 'SecToRevive',
			*
		from UsersChars where CustomerID=@in_CustomerID order by CharID asc
	end

--
-- report inventory
--
	select *
	from UsersInventory
	where CustomerID=@in_CustomerID and CharID=0

--
-- report backpacks
--
	if(@in_CharID > 0) begin
		-- single character, called from server
		select * from UsersInventory where CharID=@in_CharID
	end
	else begin
		select * from UsersInventory where CustomerID=@in_CustomerID and CharID>0 order by CharID asc
	end

END
GO


-- ----------------------------
-- procedure structure for WZ_GetAccountInfo2
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_GetAccountInfo2]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_GetAccountInfo2]
GO

CREATE PROCEDURE [dbo].[WZ_GetAccountInfo2]
	@in_CustomerID int,
	@in_CharID int
AS
BEGIN
	SET NOCOUNT ON;

	-- check if CustomerID is valid
	if not exists (SELECT CustomerID FROM UsersData WHERE CustomerID=@in_CustomerID)
	begin
		select 6 as ResultCode
		return;
	end

	if(@in_CharID > 0) begin
		update UsersData set lastjoineddate=GETDATE() where CustomerID=@in_CustomerID
	end

	select 0 as ResultCode

	SELECT
		UsersData.*,
		DATEDIFF(ss, lastgamedate, GETDATE()) as 'SecFromLastGame'
	FROM UsersData
	where UsersData.CustomerID=@in_CustomerID

--
-- report chars
--
	if(@in_CharID > 0)
	begin
		-- single character, version called from server
		select
			0 as 'SecToRevive',
			c.*,
			ClanData.ClanTag, ClanData.ClanTagColor
		from UsersChars c
		left JOIN ClanData on (c.ClanID = ClanData.ClanID)
		where CustomerID=@in_CustomerID and CharID=@in_CharID
	end
	else
	begin
		-- note that revive timer is 1hrs, change in WZ_CharRevive as well
		select
			DATEDIFF(second, GETUTCDATE(), DATEADD(hour, 1, DeathUtcTime)) as 'SecToRevive',
			c.*,
			ClanData.ClanTag, ClanData.ClanTagColor
		from UsersChars c
		left JOIN ClanData on (c.ClanID = ClanData.ClanID)
		where CustomerID=@in_CustomerID order by CharID asc
	end

--
-- report inventory
--
	select *
	from UsersInventory
	where CustomerID=@in_CustomerID and CharID=0

--
-- report backpacks
--
	if(@in_CharID > 0) begin
		-- single character, called from server
		select * from UsersInventory where CharID=@in_CharID
	end
	else begin
		select * from UsersInventory where CustomerID=@in_CustomerID and CharID>0 order by CharID asc
	end

END
GO


-- ----------------------------
-- procedure structure for WZ_GetDataGameRewards
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_GetDataGameRewards]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_GetDataGameRewards]
GO

CREATE PROCEDURE [dbo].[WZ_GetDataGameRewards]
AS
BEGIN
	SET NOCOUNT ON;

	select 0 as ResultCode
	select * from DataGameRewards
END
GO


-- ----------------------------
-- procedure structure for WZ_GetItemsData
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_GetItemsData]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_GetItemsData]
GO

CREATE PROCEDURE [dbo].[WZ_GetItemsData]
AS
BEGIN
	SET NOCOUNT ON;

	select 0 as ResultCode

	select * from Items_Gear
	select * from Items_Weapons;
	select * from Items_Generic
	select
		cast(0 as int) as ItemID,
		cast('' as nvarchar(32)) as Name,
		cast('' as varchar(32)) as FNAME,
		cast('' as nvarchar(512)) as Description,
		cast(0 as int) as AddGP,
		cast(0 as int) as AddSP,
		cast(0 as int) as Item1_ID,
		cast(0 as int) as Item1_Exp,
		cast(0 as int) as Item2_ID,
		cast(0 as int) as Item2_Exp,
		cast(0 as int) as Item3_ID,
		cast(0 as int) as Item3_Exp,
		cast(0 as int) as Item4_ID,
		cast(0 as int) as Item4_Exp,
		cast(0 as int) as Item5_ID,
		cast(0 as int) as Item5_Exp,
		cast(0 as int) as Item6_ID,
		cast(0 as int) as Item6_Exp,
		cast(0 as int) as Price1,
		cast(0 as int) as Price7,
		cast(0 as int) as Price30,
		cast(0 as int) as PriceP,
		cast(0 as int) as GPrice1,
		cast(0 as int) as GPrice7,
		cast(0 as int) as GPrice30,
		cast(0 as int) as GPriceP
	where 1 = 0

END
GO


-- ----------------------------
-- procedure structure for WZ_GetShopInfo1
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_GetShopInfo1]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_GetShopInfo1]
GO

CREATE PROCEDURE [dbo].[WZ_GetShopInfo1]
AS
BEGIN
	SET NOCOUNT ON;

	select 0 as ResultCode

	-- select all shop items
	      SELECT ItemID, Category, IsNew, Price1, Price7, Price30, PriceP, GPrice1, GPrice7, GPrice30, GPriceP FROM Items_Gear
	union SELECT ItemID, Category, IsNew, Price1, Price7, Price30, PriceP, GPrice1, GPrice7, GPrice30, GPriceP	FROM Items_Weapons
	union SELECT ItemID, Category, IsNew, Price1, Price7, Price30, PriceP, GPrice1, GPrice7, GPrice30, GPriceP	FROM Items_Generic
	union SELECT ItemID, Category, IsNew, Price1, Price7, Price30, PriceP, GPrice1, GPrice7, GPrice30, GPriceP	FROM Items_Attachments

END
GO


-- ----------------------------
-- procedure structure for WZ_LeaderboardGet
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_LeaderboardGet]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_LeaderboardGet]
GO

CREATE PROCEDURE [dbo].[WZ_LeaderboardGet]
	@in_Hardcore int,
	@in_Type int,
	@in_Page int
AS
BEGIN
	SET NOCOUNT ON;

	-- this call is always success
	SELECT 0 AS ResultCode

	DECLARE @rowsPerPage FLOAT = 100.0
	DECLARE @rowLimit INT = 2000;
	DECLARE @startPos INT = (@in_Page - 1) * @rowsPerPage

	IF @in_Type = 5 -- Heroes
	BEGIN
		SELECT TOP (@rowLimit) (@startPos + 1) AS StartPos,
			CEILING(COUNT(*) / @rowsPerPage) AS PageCount
		FROM UsersChars
		WHERE Hardcore = @in_Hardcore AND Reputation >= 0

		;WITH LeaderboardData AS
		(
			SELECT TOP (@rowLimit) Gamertag, Alive, Reputation AS Data,
				ROW_NUMBER() OVER
					(ORDER BY Reputation DESC) AS Pos
			FROM UsersChars
			WHERE Hardcore = @in_Hardcore AND Reputation >= 0
		)
		SELECT Gamertag, Alive, Data
		FROM LeaderboardData
		WHERE Pos > @startPos AND Pos <= @startPos + @rowsPerPage
	END
	ELSE IF @in_Type = 6 -- Villains
	BEGIN
		SELECT TOP (@rowLimit) (@startPos + 1) AS StartPos,
			CEILING(COUNT(*) / @rowsPerPage) AS PageCount
		FROM UsersChars
		WHERE Hardcore = @in_Hardcore AND Reputation < 0

		;WITH LeaderboardData AS
		(
			SELECT TOP (@rowLimit) Gamertag, Alive, Reputation AS Data,
				ROW_NUMBER() OVER
					(ORDER BY Reputation) AS Pos
			FROM UsersChars
			WHERE Hardcore = @in_Hardcore AND Reputation < 0
		)
		SELECT Gamertag, Alive, Data
		FROM LeaderboardData
		WHERE Pos > @startPos AND Pos <= @startPos + @rowsPerPage
	END
	ELSE
	BEGIN
		SELECT TOP (@rowLimit) (@startPos + 1) AS StartPos,
			CEILING(COUNT(*) / @rowsPerPage) AS PageCount
		FROM UsersChars
		WHERE Hardcore = @in_Hardcore

		;WITH LeaderboardData AS
		(
			SELECT TOP (@rowLimit) Gamertag, Alive,
				CASE @in_Type
					WHEN 0 THEN XP
					WHEN 1 THEN TimePlayed
					WHEN 2 THEN Stat00 -- KilledZombies
					WHEN 3 THEN Stat01 -- KilledSurvivors
					WHEN 4 THEN Stat02 -- KilledBandits
					WHEN 5 THEN Reputation
					WHEN 6 THEN Reputation
					ELSE 0
				END AS Data,
				ROW_NUMBER() OVER
					(ORDER BY
						CASE WHEN @in_Type = 0 THEN XP END DESC,
						CASE WHEN @in_Type = 1 THEN TimePlayed END DESC,
						CASE WHEN @in_Type = 2 THEN Stat00 END DESC,
						CASE WHEN @in_Type = 3 THEN Stat01 END DESC,
						CASE WHEN @in_Type = 4 THEN Stat02 END DESC,
						CASE WHEN @in_Type = 5 THEN Reputation END DESC,
						CASE WHEN @in_Type = 6 THEN Reputation END) AS Pos
			FROM UsersChars
			WHERE Hardcore = @in_Hardcore
		)
		SELECT Gamertag, Alive, Data
		FROM LeaderboardData
		WHERE Pos > @startPos AND Pos <= @startPos + @rowsPerPage
	END

END
GO


-- ----------------------------
-- procedure structure for WZ_SRV_AddCheatAttempt
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_SRV_AddCheatAttempt]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_SRV_AddCheatAttempt]
GO

CREATE PROCEDURE [dbo].[WZ_SRV_AddCheatAttempt]
	@in_IP char(32),
	@in_CustomerID int,
	@in_GameSessionID bigint,

	@in_CheatID int
AS
BEGIN
	SET NOCOUNT ON;

	INSERT INTO CheatLog (SessionID, CustomerID, CheatID, ReportTime)
	VALUES               (@in_GameSessionID, @in_CustomerID, @in_CheatID, GETDATE())

	-- we're done
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_SRV_AddLogInfo
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_SRV_AddLogInfo]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_SRV_AddLogInfo]
GO

CREATE PROCEDURE [dbo].[WZ_SRV_AddLogInfo]
	@in_CustomerID int,
	@in_CharID int = 0,
	@in_Gamertag nvarchar(64) = N'',
	@in_CustomerIP varchar(64),
	@in_GameSessionID bigint,
	@in_CheatID int,
	@in_Msg varchar(4000),
	@in_Data varchar(4000)
AS
BEGIN
	SET NOCOUNT ON;

	-- see if this event is recurring inside single game session
	--declare @RecordID int
	--select @RecordID=RecordID from DBG_SrvLogInfo where
	--	GameSessionID=@in_GameSessionID
	--	and CustomerID=@in_CustomerID
	--	and (@in_CheatID > 0 and CheatID=@in_CheatID)
	--	and @in_Msg=Msg
	--	and @in_Data=Data
	--if(@@ROWCOUNT > 0) begin
	--	-- increase count
	--	update DBG_SrvLogInfo set RepeatCount=RepeatCount+1 where RecordID=@RecordID
	--	select 0 as ResultCode
	--	return
	--end

	insert into DBG_SrvLogInfo (
		ReportTime,
		IsProcessed,
		CustomerID,
		CharID,
		Gamertag,
		CustomerIP,
		GameSessionID,
		CheatID,
		RepeatCount,
		Msg,
		Data)
	values (
		GETDATE(),
		0,
		@in_CustomerID,
		@in_CharID,
		@in_Gamertag,
		@in_CustomerIP,
		@in_GameSessionID,
		@in_CheatID,
		1,
		@in_Msg,
		@in_Data)

	-- we're done
	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_SRV_AddWeaponStats
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_SRV_AddWeaponStats]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_SRV_AddWeaponStats]
GO

CREATE PROCEDURE [dbo].[WZ_SRV_AddWeaponStats]
	@in_ItemID int,
	@in_ShotsFired int,
	@in_ShotsHits int,
	@in_KillsCQ int,
	@in_KillsDM int,
	@in_KillsSB int
AS
BEGIN
	SET NOCOUNT ON;

	update Items_Weapons set
		ShotsFired=(ShotsFired + @in_ShotsFired),
		ShotsHits=(ShotsHits + @in_ShotsHits),
		KillsCQ=(KillsCQ + @in_KillsCQ),
		KillsDM=(KillsDM + @in_KillsDM),
		KillsSB=(KillsSB + @in_KillsSB)
	where ItemID=@in_ItemID

	-- we're done
	select 0 as ResultCode

END
GO


-- ----------------------------
-- procedure structure for WZ_SRV_NoteAddNew
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_SRV_NoteAddNew]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_SRV_NoteAddNew]
GO

CREATE PROCEDURE [dbo].[WZ_SRV_NoteAddNew]
	@in_CustomerID int,
	@in_CharID int,
	@in_GameServerID int,
	@in_GamePos varchar(128),
	@in_ExpireMins int,
	@in_TextFrom nvarchar(128),
	@in_TextSubj nvarchar(2048)
AS
BEGIN
	SET NOCOUNT ON;

	insert into ServerNotesData (
		GameServerId,
		GamePos,
		CreateUtcDate,
		[ExpireUtcDate],
		CustomerID,
		CharID,
		TextFrom,
		TextSubj
	) values (
		@in_GameServerID,
		@in_GamePos,
		GETUTCDATE(),
		DATEADD(mi, @in_ExpireMins, GETUTCDATE()),
		@in_CustomerID,
		@in_CharID,
		@in_TextFrom,
		@in_TextSubj
	)
	declare @NoteID int = SCOPE_IDENTITY()

	-- we're done
	select 0 as ResultCode
	select @NoteID as 'NoteID'
END
GO


-- ----------------------------
-- procedure structure for WZ_SRV_NoteGetAll
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_SRV_NoteGetAll]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_SRV_NoteGetAll]
GO

CREATE PROCEDURE [dbo].[WZ_SRV_NoteGetAll]
	@in_GameServerID int
AS
BEGIN
	SET NOCOUNT ON;

	-- we're done
	select 0 as ResultCode

	delete from ServerNotesData where GameServerId=@in_GameServerID and GETUTCDATE()>[ExpireUtcDate]

	select GETUTCDATE() as 'CurUtcDate'
	select * from ServerNotesData where GameServerId=@in_GameServerID
END
GO


-- ----------------------------
-- procedure structure for WZ_SRV_UserJoinedGame
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_SRV_UserJoinedGame]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_SRV_UserJoinedGame]
GO

CREATE PROCEDURE [dbo].[WZ_SRV_UserJoinedGame]
	@in_CustomerID int,
	@in_CharID int,
	@in_GameMapId int,
	@in_GameServerId bigint,
	@in_GamePos varchar(256)
AS
BEGIN
	SET NOCOUNT ON;

	-- store current user server location
	update UsersData set
		lastgamedate=GETDATE(),
		GameServerId=@in_GameServerId
	where CustomerID=@in_CustomerID

	-- per char info
	update UsersChars set
		GameMapId=@in_GameMapId,
		GameServerId=@in_GameServerId
	where CharID=@in_CharID

	-- we're done
	select 0 as ResultCode

END
GO


-- ----------------------------
-- procedure structure for WZ_SRV_UserJoinedGame2
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_SRV_UserJoinedGame2]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_SRV_UserJoinedGame2]
GO

CREATE PROCEDURE [dbo].[WZ_SRV_UserJoinedGame2]
	@in_CustomerID int,
	@in_CharID int,
	@in_GameMapId int,
	@in_GameServerId bigint
AS
BEGIN
	SET NOCOUNT ON;

	-- check if game is still active or 90sec passed from last update (COPYPASTE_GAMECHECK, search for others)
	declare @lastgamedate datetime
	declare @GameServerId int
	select @GameServerId=GameServerId, @lastgamedate=lastgamedate from UsersData where CustomerID=@in_CustomerID
	if(@GameServerId > 0 and DATEDIFF(second, @lastgamedate, GETDATE()) < 90) begin
		select 7 as ResultCode, 'game still active' as ResultMsg
		return
	end

	-- store current user server location
	update UsersData set
		lastgamedate=GETDATE(),
		GameServerId=@in_GameServerId
	where CustomerID=@in_CustomerID

	-- per char info
	update UsersChars set
		GameMapId=@in_GameMapId,
		GameServerId=@in_GameServerId
	where CharID=@in_CharID

	-- we're done
	select 0 as ResultCode

END
GO


-- ----------------------------
-- procedure structure for WZ_SRV_UserLeftGame
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_SRV_UserLeftGame]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_SRV_UserLeftGame]
GO

CREATE PROCEDURE [dbo].[WZ_SRV_UserLeftGame]
	@in_CustomerID int,
	@in_CharID int,
	@in_GameMapId int,
	@in_GameServerId bigint,
	@in_TimePlayed int
AS
BEGIN
	SET NOCOUNT ON;

	-- store current user server location
	update UsersData set
		lastgamedate=GETDATE(),
		GameServerId=0,
		TimePlayed=(TimePlayed+@in_TimePlayed)
	where CustomerID=@in_CustomerID

	-- update some stats here

	-- we're done
	select 0 as ResultCode

END
GO


-- ----------------------------
-- procedure structure for WZ_UpdateAchievementStatus
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_UpdateAchievementStatus]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_UpdateAchievementStatus]
GO

CREATE PROCEDURE [dbo].[WZ_UpdateAchievementStatus]
	@in_CustomerID int,
	@in_AchID int,
	@in_AchValue int,
	@in_AchUnlocked int
AS
BEGIN
	SET NOCOUNT ON;

	if not exists (select * from Achievements where (AchID=@in_AchID and CustomerID=@in_CustomerID))
	begin
		INSERT INTO Achievements(
			CustomerID,
			AchID,
			Value,
			Unlocked
		)
		VALUES (
			@in_CustomerID,
			@in_AchID,
			@in_AchValue,
			@in_AchUnlocked
		)
	end
	else
	begin
		UPDATE Achievements SET
			Value=@in_AchValue,
			Unlocked=@in_AchUnlocked
		WHERE AchID=@in_AchID and CustomerID=@in_CustomerID
    end

    select 0 as ResultCode

    -- check for steamID
    declare @SteamID bigint = 0
	--select @SteamID=SteamID from SteamUserIDMap where CustomerID=@in_CustomerID
	--select @SteamID as 'SteamID'

END
GO


-- ----------------------------
-- procedure structure for WZ_UpdateLoginSession
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_UpdateLoginSession]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_UpdateLoginSession]
GO

CREATE PROCEDURE [dbo].[WZ_UpdateLoginSession]
	@in_IP varchar(32),
	@in_CustomerID int,
	@in_SessionID int
AS
BEGIN
	SET NOCOUNT ON;

	declare @SessionID int

	-- check if we have record for that user
	SELECT
		@SessionID=LoginSessions.SessionID
	FROM LoginSessions
	WHERE CustomerID=@in_CustomerID
	if (@@RowCount = 0) begin
		select 6 as ResultCode
		return
	end

	-- compare session key. if it's different, supplied sesson is invalid
	if(@in_SessionID <> @SessionID) begin
		select 1 as ResultCode
		return
	end

	-- update last ping time
	UPDATE LoginSessions SET
		LoginSessions.TimeUpdated=GETDATE()
	WHERE CustomerID=@in_CustomerID

	select 0 as ResultCode
END
GO


-- ----------------------------
-- procedure structure for WZ_VITALSTATS_V1
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_VITALSTATS_V1]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_VITALSTATS_V1]
GO

CREATE PROCEDURE [dbo].[WZ_VITALSTATS_V1]
AS
BEGIN

	SET NOCOUNT ON;


declare @today datetime
set @today = GETDATE()

declare @Sales int;
declare @TUsers int;
declare @DAU int;
declare @CCU int;
declare @Revenues int;

set @Sales = 0--(select count(*) from [BreezeNet.WarZPreorders])
set @Revenues = 0--(select COALESCE(SUM(Amount),0) from [BreezeNet.WarZPreorders] where OrderDate>=@today and OrderDate<DATEADD(hour, -1, @today))

set @TUsers = (select count(*) from Accounts)
set @DAU = (select count(*) from Accounts where lastlogindate > DATEADD(hour, -24, @today))
set @CCU = (select count(*) from LoginSessions where TimeUpdated > DATEADD(minute, -7, @today))

INSERT INTO VitalStats_V1 VALUES (@today, @Sales, @TUsers, @DAU, @CCU, @Revenues );


END
GO


-- ----------------------------
-- function structure for fn_diagramobjects
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[fn_diagramobjects]') AND type IN ('FN', 'FS', 'FT', 'IF', 'TF'))
	DROP FUNCTION [dbo].[fn_diagramobjects]
GO

CREATE FUNCTION [dbo].[fn_diagramobjects]()
	RETURNS int
	WITH EXECUTE AS N'dbo'
	AS
	BEGIN
		declare @id_upgraddiagrams		int
		declare @id_sysdiagrams			int
		declare @id_helpdiagrams		int
		declare @id_helpdiagramdefinition	int
		declare @id_creatediagram	int
		declare @id_renamediagram	int
		declare @id_alterdiagram 	int
		declare @id_dropdiagram		int
		declare @InstalledObjects	int

		select @InstalledObjects = 0

		select 	@id_upgraddiagrams = object_id(N'dbo.sp_upgraddiagrams'),
			@id_sysdiagrams = object_id(N'dbo.sysdiagrams'),
			@id_helpdiagrams = object_id(N'dbo.sp_helpdiagrams'),
			@id_helpdiagramdefinition = object_id(N'dbo.sp_helpdiagramdefinition'),
			@id_creatediagram = object_id(N'dbo.sp_creatediagram'),
			@id_renamediagram = object_id(N'dbo.sp_renamediagram'),
			@id_alterdiagram = object_id(N'dbo.sp_alterdiagram'),
			@id_dropdiagram = object_id(N'dbo.sp_dropdiagram')

		if @id_upgraddiagrams is not null
			select @InstalledObjects = @InstalledObjects + 1
		if @id_sysdiagrams is not null
			select @InstalledObjects = @InstalledObjects + 2
		if @id_helpdiagrams is not null
			select @InstalledObjects = @InstalledObjects + 4
		if @id_helpdiagramdefinition is not null
			select @InstalledObjects = @InstalledObjects + 8
		if @id_creatediagram is not null
			select @InstalledObjects = @InstalledObjects + 16
		if @id_renamediagram is not null
			select @InstalledObjects = @InstalledObjects + 32
		if @id_alterdiagram  is not null
			select @InstalledObjects = @InstalledObjects + 64
		if @id_dropdiagram is not null
			select @InstalledObjects = @InstalledObjects + 128

		return @InstalledObjects
	END
GO


-- ----------------------------
-- procedure structure for WZ_ACCOUNT_CREATE
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_ACCOUNT_CREATE]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_ACCOUNT_CREATE]
GO

CREATE PROCEDURE [dbo].[WZ_ACCOUNT_CREATE]
    @in_IP varchar(64),
    @in_Email varchar(128),
    @in_NickName nvarchar(64),
    @in_Password varchar(64),
    @in_ReferralID int = 0
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    SET @in_Email =
        LTRIM(
            RTRIM(
                @in_Email
            )
        );

    SET @in_NickName =
        LTRIM(
            RTRIM(
                @in_NickName
            )
        );

    /* --------------------------------------------------------
       Email
       -------------------------------------------------------- */

    IF LEN(@in_Email) < 5 OR LEN(@in_Email) > 128
    BEGIN
        SELECT
            2 AS ResultCode,
            'Invalid email address' AS ResultMsg;

        RETURN;
    END;

    IF CHARINDEX('@', @in_Email) <= 1
    BEGIN
        SELECT
            2 AS ResultCode,
            'Invalid email address' AS ResultMsg;

        RETURN;
    END;

    /* --------------------------------------------------------
       NickName
       -------------------------------------------------------- */

    IF LEN(@in_NickName) < 4 OR LEN(@in_NickName) > 16
    BEGIN
        SELECT
            9 AS ResultCode,
            'Nickname must contain between 4 and 16 characters'
                AS ResultMsg;

        RETURN;
    END;

    /*
        Р В Р В°Р В·РЎР‚Р ВµРЎв‚¬Р ВµР Р…РЎвЂ№ РЎвЂљР С•Р В»РЎРЉР С”Р С•:
        A-Z
        a-z
        0-9
    */
    IF @in_NickName COLLATE Latin1_General_100_BIN2
        LIKE '%[^A-Za-z0-9]%'
    BEGIN
        SELECT
            9 AS ResultCode,
            'Nickname contains invalid characters'
                AS ResultMsg;

        RETURN;
    END;

    IF LOWER(@in_NickName) IN
    (
        'admin',
        'administrator',
        'moderator',
        'developer',
        'server',
        'system',
        'support'
    )
    BEGIN
        SELECT
            9 AS ResultCode,
            'Nickname is reserved'
                AS ResultMsg;

        RETURN;
    END;

    IF LEN(@in_Password) < 6 OR LEN(@in_Password) > 64
    BEGIN
        SELECT
            4 AS ResultCode,
            'Password must contain between 6 and 64 characters'
                AS ResultMsg;

        RETURN;
    END;

    BEGIN TRY
        BEGIN TRANSACTION;

        /* ----------------------------------------------------
           Р РЋРЎвЂљР В°РЎР‚РЎвЂ№Р в„– refunded Р В°Р С”Р С”Р В°РЎС“Р Р…РЎвЂљ Р С•РЎРѓР Р†Р С•Р В±Р С•Р В¶Р Т‘Р В°Р ВµРЎвЂљ Email
           ---------------------------------------------------- */

        DECLARE @RefundCustomerID int = 0;

        SELECT
            @RefundCustomerID = CustomerID
        FROM dbo.Accounts WITH
        (
            UPDLOCK,
            HOLDLOCK
        )
        WHERE
            email = @in_Email
            AND AccountStatus = 999;

        IF @RefundCustomerID > 0
        BEGIN
            DECLARE @DateText varchar(32);

            SET @DateText =
                REPLACE(
                    CONVERT(
                        varchar(10),
                        GETDATE(),
                        112
                    ),
                    '/',
                    ''
                )
                +
                REPLACE(
                    CONVERT(
                        varchar(8),
                        GETDATE(),
                        108
                    ),
                    ':',
                    ''
                );

            UPDATE dbo.Accounts
            SET email =
                CONCAT(
                    '(',
                    @DateText,
                    ')',
                    CustomerID,
                    '_',
                    email
                )
            WHERE CustomerID = @RefundCustomerID;
        END;

        /* ----------------------------------------------------
           Email РЎС“Р В¶Р Вµ Р С‘РЎРѓР С—Р С•Р В»РЎРЉР В·РЎС“Р ВµРЎвЂљРЎРѓРЎРЏ
           ---------------------------------------------------- */

        IF EXISTS
        (
            SELECT 1
            FROM dbo.Accounts WITH
            (
                UPDLOCK,
                HOLDLOCK
            )
            WHERE email = @in_Email
        )
        BEGIN
            ROLLBACK TRANSACTION;

            SELECT
                2 AS ResultCode,
                'Email already in use'
                    AS ResultMsg;

            RETURN;
        END;

        /* ----------------------------------------------------
           NickName РЎС“Р В¶Р Вµ Р С‘РЎРѓР С—Р С•Р В»РЎРЉР В·РЎС“Р ВµРЎвЂљРЎРѓРЎРЏ
           ---------------------------------------------------- */

        IF EXISTS
        (
            SELECT 1
            FROM dbo.UsersChars WITH
            (
                UPDLOCK,
                HOLDLOCK
            )
            WHERE Gamertag = @in_NickName
        )
        BEGIN
            ROLLBACK TRANSACTION;

            SELECT
                9 AS ResultCode,
                'Nickname already exists'
                    AS ResultMsg;

            RETURN;
        END;

        /* ----------------------------------------------------
           Р СџР В°РЎР‚Р С•Р В»РЎРЉ
           ---------------------------------------------------- */

        DECLARE @MD5Password varchar(100);

        EXEC dbo.FN_CreateMD5Password
            @in_Password,
            @MD5Password OUTPUT;

        /* ----------------------------------------------------
           Accounts
           ---------------------------------------------------- */

        INSERT INTO dbo.Accounts
        (
            email,
            MD5Password,
            dateregistered,
            ReferralID,
            AccountStatus,
            IsDeveloper,
            lastlogindate,
            lastloginIP
        )
        VALUES
        (
            @in_Email,
            @MD5Password,
            GETDATE(),
            @in_ReferralID,
            100,
            0,
            GETDATE(),
            LEFT(
                ISNULL(
                    @in_IP,
                    '0.0.0.0'
                ),
                16
            )
        );

        DECLARE @CustomerID int =
            CONVERT(
                int,
                SCOPE_IDENTITY()
            );

        /* ----------------------------------------------------
           UsersData
           ---------------------------------------------------- */

        INSERT INTO dbo.UsersData
        (
            CustomerID,
            AccountType,
            AccountStatus,
            GamePoints,
            GameDollars,
            dateregistered,
            CharsCreated
        )
        VALUES
        (
            @CustomerID,
            0,
            100,
            4260,
            0,
            GETDATE(),
            1
        );

        /* ----------------------------------------------------
           Р СџР С•РЎРѓРЎвЂљР С•РЎРЏР Р…Р Р…РЎвЂ№Р в„– РЎРѓРЎвЂљР В°РЎР‚РЎвЂљР С•Р Р†РЎвЂ№Р в„– Р С—Р ВµРЎР‚РЎРѓР С•Р Р…Р В°Р В¶

           HeroItemID: 20754
           Name: LMS Character
           FNAME: char_lms
           ---------------------------------------------------- */

        INSERT INTO dbo.UsersChars
        (
            CustomerID,
            Hardcore,
            Gamertag,
            HeroItemID,
            HeadIdx,
            BodyIdx,
            LegsIdx,
            HairIdx,
            FeetIdx,
            BackpackID,
            BackpackSize,
            Alive,
            GameFlags,
            CreateDate
        )
        VALUES
        (
            @CustomerID,
            0,
            @in_NickName,
            20754,
            0,
            0,
            0,
            0,
            0,
            21712,
            16,
            3,
            1,
            GETDATE()
        );

        DECLARE @CharID int =
            CONVERT(
                int,
                SCOPE_IDENTITY()
            );

        /* ----------------------------------------------------
           Р РЋРЎвЂљР В°РЎР‚РЎвЂљР С•Р Р†РЎвЂ№Р Вµ Р С—РЎР‚Р ВµР Т‘Р СР ВµРЎвЂљРЎвЂ№ Р С—Р ВµРЎР‚РЎРѓР С•Р Р…Р В°Р В¶Р В°
           ---------------------------------------------------- */

        INSERT INTO dbo.UsersInventory
        (
            CustomerID,
            CharID,
            BackpackSlot,
            ItemID,
            LeasedUntil,
            Quantity,
            Var1,
            Var2
        )
        VALUES
        (
            @CustomerID,
            @CharID,
            1,
            101306,
            '2020-01-01',
            1,
            -1,
            -1
        );

        INSERT INTO dbo.UsersInventory
        (
            CustomerID,
            CharID,
            BackpackSlot,
            ItemID,
            LeasedUntil,
            Quantity,
            Var1,
            Var2
        )
        VALUES
        (
            @CustomerID,
            @CharID,
            2,
            109505,
            '2020-01-01',
            1,
            -1,
            -1
        );

        INSERT INTO dbo.UsersInventory
        (
            CustomerID,
            CharID,
            BackpackSlot,
            ItemID,
            LeasedUntil,
            Quantity,
            Var1,
            Var2
        )
        VALUES
        (
            @CustomerID,
            @CharID,
            3,
            101296,
            '2020-01-01',
            1,
            -1,
            -1
        );

        INSERT INTO dbo.UsersInventory
        (
            CustomerID,
            CharID,
            BackpackSlot,
            ItemID,
            LeasedUntil,
            Quantity,
            Var1,
            Var2
        )
        VALUES
        (
            @CustomerID,
            @CharID,
            4,
            101289,
            '2020-01-01',
            1,
            -1,
            -1
        );

        COMMIT TRANSACTION;

        SELECT
            0 AS ResultCode;

        SELECT
            @CustomerID AS CustomerID,
            @CharID AS CharID,
            0 AS AccountType;

        RETURN;
    END TRY
    BEGIN CATCH
        IF XACT_STATE() <> 0
        BEGIN
            ROLLBACK TRANSACTION;
        END;

        SELECT
            5 AS ResultCode,
            ERROR_MESSAGE() AS ResultMsg;

        RETURN;
    END CATCH;
END;
GO


-- ----------------------------
-- procedure structure for WZ_CharCreate
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_CharCreate]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_CharCreate]
GO

CREATE PROCEDURE [dbo].[WZ_CharCreate]
    @in_CustomerID int,
    @in_Hardcore int,
    @in_Gamertag nvarchar(64),
    @in_HeroItemID int,
    @in_HeadIdx int,
    @in_BodyIdx int,
    @in_LegsIdx int,
    @in_HairIdx int = 0,
    @in_FeetIdx int = 0
AS
BEGIN
    SET NOCOUNT ON;

    SELECT
        7 AS ResultCode,
        'Additional character creation is disabled'
            AS ResultMsg;
END;
GO


-- ----------------------------
-- procedure structure for WZ_CharDelete
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_CharDelete]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_CharDelete]
GO

CREATE PROCEDURE [dbo].[WZ_CharDelete]
    @in_CustomerID int,
    @in_CharID int
AS
BEGIN
    SET NOCOUNT ON;

    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.UsersChars
        WHERE
            CustomerID = @in_CustomerID
            AND CharID = @in_CharID
    )
    BEGIN
        SELECT
            6 AS ResultCode,
            'Bad character ID'
                AS ResultMsg;

        RETURN;
    END;

    SELECT
        7 AS ResultCode,
        'Permanent survivor cannot be deleted'
            AS ResultMsg;
END;
GO


-- ----------------------------
-- procedure structure for WZ_CharRename
-- ----------------------------
IF EXISTS (SELECT * FROM sys.all_objects WHERE object_id = OBJECT_ID(N'[dbo].[WZ_CharRename]') AND type IN ('P', 'PC', 'RF', 'X'))
	DROP PROCEDURE [dbo].[WZ_CharRename]
GO

CREATE PROCEDURE [dbo].[WZ_CharRename]
    @in_CustomerID int,
    @in_CharID int,
    @in_Gamertag nvarchar(64)
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    SET @in_Gamertag =
        LTRIM(
            RTRIM(
                @in_Gamertag
            )
        );

    IF LEN(@in_Gamertag) < 4 OR LEN(@in_Gamertag) > 16
    BEGIN
        SELECT
            9 AS ResultCode,
            'Nickname must contain between 4 and 16 characters'
                AS ResultMsg;

        RETURN;
    END;

    IF @in_Gamertag COLLATE Latin1_General_100_BIN2
        LIKE '%[^A-Za-z0-9]%'
    BEGIN
        SELECT
            9 AS ResultCode,
            'Nickname contains invalid characters'
                AS ResultMsg;

        RETURN;
    END;

    IF LOWER(@in_Gamertag) IN
    (
        'admin',
        'administrator',
        'moderator',
        'developer',
        'server',
        'system',
        'support'
    )
    BEGIN
        SELECT
            9 AS ResultCode,
            'Nickname is reserved'
                AS ResultMsg;

        RETURN;
    END;

    BEGIN TRY
        BEGIN TRANSACTION;

        IF NOT EXISTS
        (
            SELECT 1
            FROM dbo.UsersChars WITH
            (
                UPDLOCK,
                HOLDLOCK
            )
            WHERE
                CustomerID = @in_CustomerID
                AND CharID = @in_CharID
        )
        BEGIN
            ROLLBACK TRANSACTION;

            SELECT
                6 AS ResultCode,
                'Bad character ID'
                    AS ResultMsg;

            RETURN;
        END;

        IF EXISTS
        (
            SELECT 1
            FROM dbo.UsersChars WITH
            (
                UPDLOCK,
                HOLDLOCK
            )
            WHERE
                Gamertag = @in_Gamertag
                AND CharID <> @in_CharID
        )
        BEGIN
            ROLLBACK TRANSACTION;

            SELECT
                9 AS ResultCode,
                'Nickname already exists'
                    AS ResultMsg;

            RETURN;
        END;

        UPDATE dbo.UsersChars
        SET Gamertag = @in_Gamertag
        WHERE
            CustomerID = @in_CustomerID
            AND CharID = @in_CharID;

        COMMIT TRANSACTION;

        SELECT
            0 AS ResultCode;

        RETURN;
    END TRY
    BEGIN CATCH
        IF XACT_STATE() <> 0
        BEGIN
            ROLLBACK TRANSACTION;
        END;

        SELECT
            5 AS ResultCode,
            ERROR_MESSAGE() AS ResultMsg;

        RETURN;
    END CATCH;
END;
GO


-- ----------------------------
-- Auto increment value for Accounts
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[Accounts]', RESEED, 1000000)
GO


-- ----------------------------
-- Indexes structure for table Accounts
-- ----------------------------
CREATE UNIQUE NONCLUSTERED INDEX [IX_Accounts_email]
ON [dbo].[Accounts] (
  [email] ASC
)
GO

CREATE UNIQUE NONCLUSTERED INDEX [UX_Accounts_Email]
ON [dbo].[Accounts] (
  [email] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table Accounts
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Accounts]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Accounts] ADD CONSTRAINT [PK__Accounts__A4AE64B8B0DE684C] PRIMARY KEY CLUSTERED ([CustomerID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Indexes structure for table Achievements
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_Achievements_AchID]
ON [dbo].[Achievements] (
  [CustomerID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_Achievements_CustomerID]
ON [dbo].[Achievements] (
  [CustomerID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table CharsStats
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[CharsStats]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[CharsStats] ADD CONSTRAINT [PK__CharsSta__AA7BC254060E9AFF] PRIMARY KEY CLUSTERED ([CharID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for CheatLog
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[CheatLog]', RESEED, 1)
GO


-- ----------------------------
-- Primary Key structure for table CheatLog
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[CheatLog]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[CheatLog] ADD CONSTRAINT [PK__CheatLog__3214EC27AD854B8A] PRIMARY KEY CLUSTERED ([ID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for ClanApplications
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[ClanApplications]', RESEED, 1)
GO


-- ----------------------------
-- Indexes structure for table ClanApplications
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_ClanApplications_CharID]
ON [dbo].[ClanApplications] (
  [CharID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_ClanApplications_ClanID]
ON [dbo].[ClanApplications] (
  [ClanID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table ClanApplications
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[ClanApplications]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[ClanApplications] ADD CONSTRAINT [PK__ClanAppl__581126780A62D8BB] PRIMARY KEY CLUSTERED ([ClanApplicationID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for ClanData
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[ClanData]', RESEED, 1472)
GO


-- ----------------------------
-- Primary Key structure for table ClanData
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[ClanData]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[ClanData] ADD CONSTRAINT [PK__ClanData__EC03AA443BA8773C] PRIMARY KEY CLUSTERED ([ClanID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for ClanEvents
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[ClanEvents]', RESEED, 1)
GO


-- ----------------------------
-- Indexes structure for table ClanEvents
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_ClanEvents_ClanID]
ON [dbo].[ClanEvents] (
  [ClanID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table ClanEvents
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[ClanEvents]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[ClanEvents] ADD CONSTRAINT [PK__ClanEven__DEFA879C27361266] PRIMARY KEY CLUSTERED ([ClanEventID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for ClanInvites
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[ClanInvites]', RESEED, 1)
GO


-- ----------------------------
-- Indexes structure for table ClanInvites
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_ClanInvites_CustomerID]
ON [dbo].[ClanInvites] (
  [CharID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_ClanInvites_ClanID]
ON [dbo].[ClanInvites] (
  [ClanID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_ClanInvites_ExpireTime]
ON [dbo].[ClanInvites] (
  [ExpireTime] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table ClanInvites
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[ClanInvites]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[ClanInvites] ADD CONSTRAINT [PK__ClanInvi__E1FC20BD1DD3EA00] PRIMARY KEY CLUSTERED ([ClanInviteID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Primary Key structure for table DataGameRewards
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[DataGameRewards]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[DataGameRewards] ADD CONSTRAINT [PK__DataGame__3214EC27CBD420B5] PRIMARY KEY CLUSTERED ([ID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Primary Key structure for table DataSkill2Price
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[DataSkill2Price]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[DataSkill2Price] ADD CONSTRAINT [PK__DataSkil__DFA091E756613E10] PRIMARY KEY CLUSTERED ([SkillID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for DBG_BanLog
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[DBG_BanLog]', RESEED, 1)
GO


-- ----------------------------
-- Auto increment value for DBG_GPTransactions
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[DBG_GPTransactions]', RESEED, 4)
GO


-- ----------------------------
-- Primary Key structure for table DBG_GPTransactions
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[DBG_GPTransactions]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[DBG_GPTransactions] ADD CONSTRAINT [PK__DBG_GPTr__55433A4B84E4D86E] PRIMARY KEY CLUSTERED ([TransactionID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Primary Key structure for table DBG_IISApiStats
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[DBG_IISApiStats]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[DBG_IISApiStats] ADD CONSTRAINT [PK__DBG_IISA__C690DF23040CDCB5] PRIMARY KEY CLUSTERED ([API])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Indexes structure for table DBG_LevelUpEvents
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_DBG_LevelUpEvents]
ON [dbo].[DBG_LevelUpEvents] (
  [CustomerID] ASC
)
GO


-- ----------------------------
-- Auto increment value for DBG_LootRewards
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[DBG_LootRewards]', RESEED, 1)
GO


-- ----------------------------
-- Auto increment value for DBG_PasswordResets
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[DBG_PasswordResets]', RESEED, 1)
GO


-- ----------------------------
-- Primary Key structure for table DBG_PasswordResets
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[DBG_PasswordResets]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[DBG_PasswordResets] ADD CONSTRAINT [PK__DBG_Pass__783CF7AD4DA8E84C] PRIMARY KEY CLUSTERED ([ResetID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for DBG_SrvLogInfo
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[DBG_SrvLogInfo]', RESEED, 8)
GO


-- ----------------------------
-- Indexes structure for table DBG_UserRoundResults
-- ----------------------------
CREATE NONCLUSTERED INDEX [DBG_URR_CID_GRP]
ON [dbo].[DBG_UserRoundResults] (
  [CustomerID] ASC,
  [GameReportTime] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_DBG_UserRoundResults_LBIdx1]
ON [dbo].[DBG_UserRoundResults] (
  [GameReportTime] ASC
)
INCLUDE ([CustomerID], [Deaths], [HonorPoints], [Kills], [Losses], [ShotsFired], [ShotsHits], [TimePlayed], [Wins])
GO


-- ----------------------------
-- Auto increment value for DBG_WOAdminChanges
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[DBG_WOAdminChanges]', RESEED, 1)
GO


-- ----------------------------
-- Indexes structure for table FinancialTransactions
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_FinancialTransactions_CustomerID]
ON [dbo].[FinancialTransactions] (
  [CustomerID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_FinancialTransactions_DateTime]
ON [dbo].[FinancialTransactions] (
  [DateTime] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_FinancialTransactions_ItemID]
ON [dbo].[FinancialTransactions] (
  [ItemID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_FinancialTransactions_TransactionType]
ON [dbo].[FinancialTransactions] (
  [TransactionType] ASC
)
GO


-- ----------------------------
-- Indexes structure for table FriendsMap
-- ----------------------------
CREATE NONCLUSTERED INDEX [IDX_FriendsMap_CustomerID]
ON [dbo].[FriendsMap] (
  [CustomerID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IDX_FriendsMap_FriendID]
ON [dbo].[FriendsMap] (
  [FriendID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table Items_Attachments
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Items_Attachments]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Items_Attachments] ADD CONSTRAINT [PK__Items_At__727E83EBA918A1E5] PRIMARY KEY CLUSTERED ([ItemID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Primary Key structure for table Items_Gear
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Items_Gear]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Items_Gear] ADD CONSTRAINT [PK__Items_Ge__727E83EBCA3951D6] PRIMARY KEY CLUSTERED ([ItemID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Primary Key structure for table Items_Generic
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Items_Generic]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Items_Generic] ADD CONSTRAINT [PK__Items_Ge__727E83EBDA6A021A] PRIMARY KEY CLUSTERED ([ItemID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for Items_LootData
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[Items_LootData]', RESEED, 142)
GO


-- ----------------------------
-- Primary Key structure for table Items_Weapons
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Items_Weapons]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Items_Weapons] ADD CONSTRAINT [PK__Items_We__727E83EB5034428B] PRIMARY KEY CLUSTERED ([ItemID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for Leaderboard
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[Leaderboard]', RESEED, 1)
GO


-- ----------------------------
-- Indexes structure for table Leaderboard
-- ----------------------------
CREATE UNIQUE NONCLUSTERED INDEX [IX_Leaderboard_CustomerID]
ON [dbo].[Leaderboard] (
  [CustomerID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table Leaderboard
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Leaderboard]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Leaderboard] ADD CONSTRAINT [PK__Leaderbo__C570494845918EA5] PRIMARY KEY CLUSTERED ([Pos])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for Leaderboard1
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[Leaderboard1]', RESEED, 1)
GO


-- ----------------------------
-- Indexes structure for table Leaderboard1
-- ----------------------------
CREATE UNIQUE NONCLUSTERED INDEX [IX_Leaderboard1_CustomerID]
ON [dbo].[Leaderboard1] (
  [CustomerID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table Leaderboard1
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Leaderboard1]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Leaderboard1] ADD CONSTRAINT [PK__Leaderbo__C5704948FA123E6C] PRIMARY KEY CLUSTERED ([Pos])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for Leaderboard30
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[Leaderboard30]', RESEED, 1)
GO


-- ----------------------------
-- Indexes structure for table Leaderboard30
-- ----------------------------
CREATE UNIQUE NONCLUSTERED INDEX [IX_Leaderboard30_CustomerID]
ON [dbo].[Leaderboard30] (
  [CustomerID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table Leaderboard30
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Leaderboard30]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Leaderboard30] ADD CONSTRAINT [PK__Leaderbo__C5704948181769EA] PRIMARY KEY CLUSTERED ([Pos])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for Leaderboard7
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[Leaderboard7]', RESEED, 1)
GO


-- ----------------------------
-- Indexes structure for table Leaderboard7
-- ----------------------------
CREATE UNIQUE NONCLUSTERED INDEX [IX_Leaderboard7_CustomerID]
ON [dbo].[Leaderboard7] (
  [CustomerID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table Leaderboard7
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Leaderboard7]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Leaderboard7] ADD CONSTRAINT [PK__Leaderbo__C5704948EB679321] PRIMARY KEY CLUSTERED ([Pos])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for Logins
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[Logins]', RESEED, 225)
GO


-- ----------------------------
-- Indexes structure for table Logins
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_Logins_CustomerID_LoginTime]
ON [dbo].[Logins] (
  [CustomerID] ASC,
  [LoginTime] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table Logins
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[Logins]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[Logins] ADD CONSTRAINT [PK__Logins__4DDA283838F352C2] PRIMARY KEY CLUSTERED ([LoginID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Primary Key structure for table LoginSessions
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[LoginSessions]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[LoginSessions] ADD CONSTRAINT [PK__LoginSes__A4AE64B8F7320361] PRIMARY KEY CLUSTERED ([CustomerID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Primary Key structure for table MasterServerInfo
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[MasterServerInfo]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[MasterServerInfo] ADD CONSTRAINT [PK__MasterSe__C56AC886D304456A] PRIMARY KEY CLUSTERED ([ServerID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for SecurityLog
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[SecurityLog]', RESEED, 1)
GO


-- ----------------------------
-- Primary Key structure for table SecurityLog
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[SecurityLog]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[SecurityLog] ADD CONSTRAINT [PK__Security__3214EC2734928AFF] PRIMARY KEY CLUSTERED ([ID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for ServerNotesData
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[ServerNotesData]', RESEED, 2)
GO


-- ----------------------------
-- Indexes structure for table ServerNotesData
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_ServerNotesData_GameServerId]
ON [dbo].[ServerNotesData] (
  [GameServerId] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table ServerNotesData
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[ServerNotesData]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[ServerNotesData] ADD CONSTRAINT [PK__ServerNo__EACE357F0213EBC8] PRIMARY KEY CLUSTERED ([NoteID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for UsersChars
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[UsersChars]', RESEED, 3)
GO


-- ----------------------------
-- Indexes structure for table UsersChars
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_UsersChars_ClanID]
ON [dbo].[UsersChars] (
  [ClanID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_UsersChars_Gamertag]
ON [dbo].[UsersChars] (
  [Gamertag] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_Profile_Loadouts2_CustomerID]
ON [dbo].[UsersChars] (
  [CustomerID] ASC
)
GO

CREATE UNIQUE NONCLUSTERED INDEX [UX_UsersChars_Gamertag]
ON [dbo].[UsersChars] (
  [Gamertag] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table UsersChars
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[UsersChars]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[UsersChars] ADD CONSTRAINT [PK__UsersCha__AA7BC254FE43FB8F] PRIMARY KEY CLUSTERED ([CharID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Indexes structure for table UsersData
-- ----------------------------
CREATE NONCLUSTERED INDEX [IX_LoginID_ClanID]
ON [dbo].[UsersData] (
  [ClanID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table UsersData
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[UsersData]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[UsersData] ADD CONSTRAINT [PK__UsersDat__A4AE64B89F077481] PRIMARY KEY CLUSTERED ([CustomerID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO


-- ----------------------------
-- Auto increment value for UsersInventory
-- ----------------------------
DBCC CHECKIDENT ('[dbo].[UsersInventory]', RESEED, 17)
GO


-- ----------------------------
-- Indexes structure for table UsersInventory
-- ----------------------------
CREATE NONCLUSTERED INDEX [x_UsersInventory_Index_Fuck]
ON [dbo].[UsersInventory] (
  [CharID] ASC
)
INCLUDE ([BackpackSlot], [CustomerID], [InventoryID], [ItemID], [LeasedUntil], [Quantity], [Var1], [Var2])
GO

CREATE NONCLUSTERED INDEX [IX_UsersInventory_CharID]
ON [dbo].[UsersInventory] (
  [CharID] ASC
)
GO

CREATE NONCLUSTERED INDEX [IX_Inventory_CustomerID]
ON [dbo].[UsersInventory] (
  [CustomerID] ASC
)
GO


-- ----------------------------
-- Primary Key structure for table UsersInventory
-- ----------------------------
IF NOT EXISTS (
    SELECT 1
    FROM sys.key_constraints
    WHERE parent_object_id = OBJECT_ID(N'[dbo].[UsersInventory]')
      AND [type] = 'PK'
)
BEGIN
    ALTER TABLE [dbo].[UsersInventory] ADD CONSTRAINT [PK__UsersInv__F5FDE6D35B217B6D] PRIMARY KEY CLUSTERED ([InventoryID])
    WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON)
    ON [PRIMARY]
END
GO
