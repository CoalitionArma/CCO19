[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_LinearAASGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_LinearAASGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("US", "auto", "The side designated as blufor", category: "Linear AAS Player Settings")]
	FactionKey bluforSide;
	
	[Attribute("SFRV", "auto", "Nickname for the side designated as blufor", category: "Linear AAS Player Settings")]
	string bluforSideNickname;
	
	[Attribute("USSR", "auto", "The side designated as opfor", category: "Linear AAS Player Settings")]
	FactionKey opforSide;
	
	[Attribute("OCP", "auto", "Nickname for the side designated as blufor", category: "Linear AAS Player Settings")]
	string opforSideNickname;
	
	[Attribute("", UIWidgets.EditBox, desc: "Array of all zone object names", category: "Linear AAS Zone Settings")]
	ref array<string> m_aZoneObjectNames;
	
	[RplProp(onRplName: "UpdateClients"), Attribute("", UIWidgets.EditBox, desc: "Array of zone status's at mission start. Example from CCO: \n\n 'US:Locked', \n 'US:Locked', \n 'N/A:Locked', \n 'USSR:Locked', \n 'USSR:Locked' \n\n Each line above represents an index in the array, index 1 is zone A, index 2 is Zone B, etc. The value is bassically: FactionKey:Locked/Unlocked", category: "Linear AAS Zone Settings")]
	ref array<string> m_aZonesStatus;
	
	[Attribute("10", "auto", "Min number of players needed to cap a zone", category: "Linear AAS Zone Settings")]
	int m_iMinNumberOfPlayersNeeded;
	
	// - All players withing a zones range
	ref array<SCR_ChimeraCharacter> m_aAllPlayersWithinZoneRange = new array<SCR_ChimeraCharacter>;
	
	[RplProp()]
	string hudMessage;
	
	int InitialTime = 20;
	
	// Zone countdown vars 
	int aZoneCountdown = 1200;
	int bZoneCountdown = 1200;
	int cZoneCountdown = 1200;
	int dZoneCountdown = 1200;
	int eZoneCountdown = 1200;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------

	static CRF_LinearAASGameModeComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_LinearAASGameModeComponent.Cast(gameMode.FindComponent(CRF_LinearAASGameModeComponent));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		GetGame().GetCallqueue().CallLater(CheckAddInitialMarkers, 1000, true);

		//--- Server only
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		GetGame().GetCallqueue().CallLater(UpdateZones, 1000, true);
		GetGame().GetCallqueue().CallLater(StartGame, 1000, true);
	}
	
	//------------------------------------------------------------------------------------------------

	// Linear AAS functions

	//------------------------------------------------------------------------------------------------
	
	void StartGame()
	{
		CRF_SafestartGameModeComponent safestart = CRF_SafestartGameModeComponent.GetInstance();
		if(safestart.GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning())
			return;
		
		if(InitialTime != 0)
		{
			m_aZonesStatus.Set(((m_aZonesStatus.Count()-1)/2), string.Format("%1:%2:%3", "N/A", InitialTime, "N/A"));
			hudMessage = string.Format("Zone C Unlocked: %1", SCR_FormatHelper.FormatTime(InitialTime));
			InitialTime = InitialTime - 1;
			Replication.BumpMe();
			return;
		}	
		
		m_aZonesStatus.Set(((m_aZonesStatus.Count()-1)/2), string.Format("%1:%2:%3", "N/A", "Unlocked", "N/A"));
		hudMessage = "Zone C Unlocked!";
		UpdateClients();
		Replication.BumpMe();
		
		GetGame().GetCallqueue().Remove(StartGame);
		
		GetGame().GetCallqueue().CallLater(ResetMessage, 6000);
		
		GetGame().GetCallqueue().CallLater(UnlockZone, 1000, true, 1, 1200);
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetMessage()
	{
		hudMessage = "";
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void CheckAddInitialMarkers()
	{
		// Create markers on each bomb site
		CRF_GameModePlayerComponent gameModePlayerComponent = CRF_GameModePlayerComponent.GetInstance();
		if (!gameModePlayerComponent) 
			return;
		
		gameModePlayerComponent.UpdateMapMarkers(m_aZonesStatus, m_aZoneObjectNames, bluforSide, opforSide);
		
		GetGame().GetCallqueue().Remove(CheckAddInitialMarkers);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateZones()
	{
		CRF_SafestartGameModeComponent safestart = CRF_SafestartGameModeComponent.GetInstance();
		if(safestart.GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning())
			return;
		
		foreach(int i, string zoneName : m_aZoneObjectNames)
		{
			m_aAllPlayersWithinZoneRange.Clear();
			
			IEntity zone = GetGame().GetWorld().FindEntityByName(zoneName);
			
			if(!zone)
				continue;
			
			GetGame().GetWorld().QueryEntitiesBySphere(zone.GetOrigin(), 75, ProcessEntity, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT); // get all entitys withing a 50m radius around the zone
			
			float bluforInZone = 0;
			float opforInZone = 0;
			
			foreach(SCR_ChimeraCharacter player : m_aAllPlayersWithinZoneRange) 
			{
				if(player.GetFactionKey() == bluforSide)
					bluforInZone = bluforInZone + 1;
				
				if(player.GetFactionKey() == opforSide)
					opforInZone = opforInZone + 1;
			};
			
			string status = m_aZonesStatus[i];
			
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);
			
			FactionKey zoneFaction = zoneStatusArray[0];
			string zoneState = zoneStatusArray[1];
			FactionKey zoneFactionStored = zoneStatusArray[2];
			
			if (bluforInZone >= m_iMinNumberOfPlayersNeeded && opforInZone < (bluforInZone/2))
			{
				CheckStartCaptureZone(i, bluforSide, zoneFaction, zoneState, zoneFactionStored);
				return;
			}
			
			if (opforInZone >= m_iMinNumberOfPlayersNeeded && bluforInZone < (opforInZone/2)) 
			{
				CheckStartCaptureZone(i, opforSide, zoneFaction, zoneState, zoneFactionStored);
				return;
			}
			
			if(zoneState.ToInt() != 0)  
			{
				m_aZonesStatus.Set(i, string.Format("%1:%2:%3", zoneFactionStored, "Unlocked", zoneFactionStored));
				UpdateClients();
				Replication.BumpMe();
			};
		};
	};
	
	//------------------------------------------------------------------------------------------------
	protected bool ProcessEntity(IEntity processEntity)
	{
		SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(processEntity);
		if (!playerCharacter) 
			return true;

		int processEntityID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(playerCharacter);
		if (!processEntityID) 
			return true;

		m_aAllPlayersWithinZoneRange.Insert(playerCharacter);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CheckStartCaptureZone(int zoneIndex, FactionKey side, FactionKey zoneFaction, string zoneState, FactionKey zoneFactionStored)
	{	
		if (zoneState == "Locked" || (zoneFaction == side && zoneState == "Unlocked")) 
			return;
		
		if(zoneState.ToInt() >= 60) // Zone officially captured
		{
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", side, "Locked", side));
			int countDown;
			string textName;
			switch (zoneIndex)
			{
				case 0:
				{
					countDown = aZoneCountdown;
					textName = "A Site";
					break;
				};
				case 1:
				{
					countDown = bZoneCountdown;
					textName = "B Site";
					break;
				};
				case 2:
				{
					countDown = cZoneCountdown;
					textName = "C Site";
					break;
				};
				case 3:
				{
					countDown = dZoneCountdown;
					textName = "D Site";
					break;
				};
				case 4:
				{
					countDown = eZoneCountdown;
					textName = "E Site";
					break;
				};
			} 
			GetGame().GetCallqueue().CallLater(UnlockZone, 1000, true, zoneIndex, countDown, textName, side);
			ZoneCaptured(zoneIndex, side);
			return;
		};
		 
		m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", side, zoneState.ToInt() + 1, zoneFactionStored));
		
		UpdateClients();
		Replication.BumpMe();
	};
	
	void ZoneCaptured(int zoneIndex, FactionKey side)
	{
		string zonesStatusToChange;
		string zonePretty;
		string nickname;
		int zoneIndexToChange;
			
		switch(zoneIndex)
		{
			case 0 : {zonePretty = "A"; break;};
			case 1 : {zonePretty = "B"; break;};
			case 2 : {zonePretty = "C"; break;};
			case 3 : {zonePretty = "D"; break;};
			case 4 : {zonePretty = "E"; break;};
		}
			
		switch(side)
		{
			case bluforSide : { nickname = bluforSideNickname; zoneIndexToChange = zoneIndex + 1; break;}; //Blufor
			case opforSide  : { nickname = opforSideNickname;  zoneIndexToChange = zoneIndex - 1; break;}; //Opfor
		}
		
		hudMessage = string.Format("%1 Captured Zone %2!", nickname, zonePretty);
		
		GetGame().GetCallqueue().CallLater(ResetMessage, 6000);
		
		string zoneStatusToChange = m_aZonesStatus.Get(zoneIndexToChange);
		
		array<string> zoneStatusToChangeArray = {};
		zoneStatusToChange.Split(":", zoneStatusToChangeArray, false);
		
		m_aZonesStatus.Set(zoneIndexToChange, string.Format("%1:%2:%3", zoneStatusToChangeArray[0], "Unlocked", zoneStatusToChangeArray[2]));
		UpdateClients();
		Replication.BumpMe();
	}
	
	void UnlockZone(int zoneIndex, int countDown, string textName, string side)
	{
		// Finished
		if (timeToUnlock <= 0) 
		{
			hudMessage = textName + " is now unlocked!";
			int aZoneCountdown = 1200;
			int bZoneCountdown = 1200;
			int cZoneCountdown = 1200;
			int dZoneCountdown = 1200;
			int eZoneCountdown = 1200;
			
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", side, "Unlocked", side));
			Replication.BumpMe();
		}
		
		// Adjust the text for site unlock every 5 mins
		if (Math.Mod(countDown, 300) == 0) {
			switch (countDown)
			{
				case 1200: {
					hudMessage = textName + " unlocks in 20 minute(s)";
					break;
				};
				case 900: {
					hudMessage = textName + " unlocks in 15 minute(s)";
					break;
				};
				case 600: {
					hudMessage = textName + " unlocks in 10 minute(s)";
					break;
				};
				case 300: {
					hudMessage = textName + " unlocks in 5 minute(s)";
					break;
				}
			}			
			Replication.BumpMe();
		}
			
		// countDown
		countDown--;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateClients()
	{
		CRF_GameModePlayerComponent.GetInstance().UpdateMapMarkers(m_aZonesStatus, m_aZoneObjectNames, bluforSide, opforSide);
	}
};