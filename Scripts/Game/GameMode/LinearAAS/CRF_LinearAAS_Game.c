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
	
	[RplProp(onRplName: "UpdateClients")]
	string hudMessage;
	
	int m_iInitialTime = 60;
	int m_iTimeToWin = 180;
	bool m_bGameStarted = false;
	int m_iZoneCaptureInProgress = -1;
	int zoneCountdown = 60;
	
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
		
		if(m_iInitialTime != 0)
		{
			m_aZonesStatus.Set(((m_aZonesStatus.Count()-1)/2), string.Format("%1:%2:%3", "N/A", m_iInitialTime, "N/A"));
			hudMessage = string.Format("Zone C Unlocked: %1", SCR_FormatHelper.FormatTime(m_iInitialTime));
			m_iInitialTime = m_iInitialTime - 1;
			Replication.BumpMe();
			return;
		};
		
		m_aZonesStatus.Set(((m_aZonesStatus.Count()-1)/2), string.Format("%1:%2:%3", "N/A", "Unlocked", "N/A"));
		hudMessage = "Zone C Unlocked!";
		m_bGameStarted = true;
		UpdateClients();
		Replication.BumpMe();
		
		GetGame().GetCallqueue().Remove(StartGame);
		
		GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
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
		if(safestart.GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning() || !m_bGameStarted)
			return;
		
		int zonesCapturedBlufor;
		int zonesCapturedOpfor;
		int zonesFullyCapturedBlufor;
		int zonesFullyCapturedOpfor;
		
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
			}
			
			string status = m_aZonesStatus[i];
			
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);
			
			FactionKey zoneFaction = zoneStatusArray[0];
			string zoneState = zoneStatusArray[1];
			FactionKey zoneFactionStored = zoneStatusArray[2];
			
			if(zoneFaction == bluforSide)
				zonesCapturedBlufor = zonesCapturedBlufor + 1;
			
			if(zoneFaction == opforSide && zoneFaction)
				zonesCapturedOpfor = zonesCapturedOpfor + 1;
			
			if(zoneFactionStored == bluforSide)
				zonesFullyCapturedBlufor = zonesFullyCapturedBlufor + 1;
		
			if(zoneFactionStored == bluforSide)
				zonesFullyCapturedOpfor = zonesFullyCapturedOpfor + 1;
			
			if (bluforInZone >= m_iMinNumberOfPlayersNeeded && opforInZone < (bluforInZone/2))
			{
				CheckStartCaptureZone(i, bluforSide, zoneFaction, zoneState, zoneFactionStored);
				continue;
			};
			
			if (opforInZone >= m_iMinNumberOfPlayersNeeded && bluforInZone < (opforInZone/2)) 
			{
				CheckStartCaptureZone(i, opforSide, zoneFaction, zoneState, zoneFactionStored);
				continue;
			};
			
			if(zoneState.ToInt() != 0)  
			{
				m_aZonesStatus.Set(i, string.Format("%1:%2:%3", zoneFactionStored, "Unlocked", zoneFactionStored));
				UpdateClients();
				Replication.BumpMe();
			};
		}
		
		if(zonesCapturedBlufor == m_aZoneObjectNames.Count())
		{
			m_iTimeToWin = m_iTimeToWin - 1;
			
			if(m_iTimeToWin <= 0)
				hudMessage = string.Format("%1 Vicory!", bluforSideNickname);
			else
				hudMessage = string.Format("%1 Vicory In: %2", bluforSideNickname, SCR_FormatHelper.FormatTime(m_iTimeToWin));
			
			UpdateClients();
			Replication.BumpMe();
			
			return;
		};
		
		if(zonesCapturedOpfor == m_aZoneObjectNames.Count())
		{
			m_iTimeToWin = m_iTimeToWin - 1;

			if(m_iTimeToWin <= 0)
				hudMessage = string.Format("%1 Vicory!", opforSideNickname);
			else
				hudMessage = string.Format("%1 Vicory In: %2", opforSideNickname, SCR_FormatHelper.FormatTime(m_iTimeToWin));
			
			UpdateClients();
			Replication.BumpMe();
			
			return;
		};
		
		if(zonesFullyCapturedBlufor != m_aZoneObjectNames.Count() && zonesFullyCapturedOpfor != m_aZoneObjectNames.Count())
			m_iTimeToWin = 180;
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
		if (zoneState == "Locked" || (zoneFaction == side && zoneState == "Unlocked") || ((m_iZoneCaptureInProgress != zoneIndex) && (m_iZoneCaptureInProgress != -1))) 
			return;
		
		if(zoneState.ToInt() >= 60) // Zone officially captured
		{
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", side, "Locked", side));
			
			ZoneCaptured(zoneIndex, side);
			m_iZoneCaptureInProgress = -1;
			return;
		};
		 
		m_iZoneCaptureInProgress = zoneIndex;
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
		int zoneIndexBehind;
			
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
			case bluforSide : { nickname = bluforSideNickname; zoneIndexToChange = zoneIndex + 1; zoneIndexBehind = zoneIndex - 1; break;}; //Blufor
			case opforSide  : { nickname = opforSideNickname;  zoneIndexToChange = zoneIndex - 1; zoneIndexBehind = zoneIndex + 1; break;}; //Opfor
		}
		
		hudMessage = string.Format("%1 Captured Zone %2!", nickname, zonePretty);
		Replication.BumpMe();
		
		GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
		
		if(zoneIndexBehind > (m_aZoneObjectNames.Count() - 1) || zoneIndexBehind < 0)
			return;	
		
		string zoneBehindToChange = m_aZonesStatus.Get(zoneIndexBehind);
		
		array<string> zoneStatusBehindArray = {};
		zoneBehindToChange.Split(":", zoneStatusBehindArray, false);
		
		m_aZonesStatus.Set(zoneIndexBehind, string.Format("%1:%2:%3", zoneStatusBehindArray[0], "Locked", zoneStatusBehindArray[2]));
		
		if(zoneIndexToChange > (m_aZoneObjectNames.Count() - 1) || zoneIndexToChange < 0)
		{
			GetGame().GetCallqueue().CallLater(UnlockZone, 1000, true, zoneIndex, zoneIndex);
			return;	
		};
		
		string zoneStatusToChange = m_aZonesStatus.Get(zoneIndexToChange);
		
		array<string> zoneStatusToChangeArray = {};
		zoneStatusToChange.Split(":", zoneStatusToChangeArray, false);
		
		m_aZonesStatus.Set(zoneIndexToChange, string.Format("%1:%2:%3", zoneStatusToChangeArray[0], "Locked", zoneStatusToChangeArray[2]));
		
		GetGame().GetCallqueue().CallLater(UnlockZone, 1000, true, zoneIndex, zoneIndexToChange);
		UpdateClients();
	}
	
	void UnlockZone(int zoneIndex, int zoneIndexTwo)
	{	
		if (zoneCountdown > 0) 
			zoneCountdown = zoneCountdown - 1;
		
		string zonePretty;
		string zonePrettyTwo;
		
		switch(zoneIndex)
		{
			case 0 : {zonePretty = "Zone A"; break;};
			case 1 : {zonePretty = "Zone B"; break;};
			case 2 : {zonePretty = "Zone C"; break;};
			case 3 : {zonePretty = "Zone D"; break;};
			case 4 : {zonePretty = "Zone E"; break;};
		}
		
		switch(zoneIndexTwo)
		{
			case 0 : {zonePrettyTwo = "Zone A"; break;};
			case 1 : {zonePrettyTwo = "Zone B"; break;};
			case 2 : {zonePrettyTwo = "Zone C"; break;};
			case 3 : {zonePrettyTwo = "Zone D"; break;};
			case 4 : {zonePrettyTwo = "Zone E"; break;};
		}
		
		if(zoneCountdown <= 0)
		{
			zoneCountdown = 60;
			
			string status = m_aZonesStatus[zoneIndex];
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);
		
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", zoneStatusArray[2], "Unlocked", zoneStatusArray[2]));
		
			string statusTwo = m_aZonesStatus[zoneIndexTwo];
			array<string> zoneStatusTwoArray = {};
			statusTwo.Split(":", zoneStatusTwoArray, false);
		
			m_aZonesStatus.Set(zoneIndexTwo, string.Format("%1:%2:%3", zoneStatusTwoArray[2], "Unlocked", zoneStatusTwoArray[2]));
			 
			if(zonePretty != zonePrettyTwo)
				hudMessage = zonePretty + " & " + zonePrettyTwo + " are now unlocked!";
			else
				hudMessage = zonePretty + " is now unlocked!";
			
			UpdateClients();
			Replication.BumpMe();
			
			GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
			GetGame().GetCallqueue().Remove(UnlockZone);
			return;
		};

		switch (zoneCountdown)
		{
			case 1200: {
				if(zonePretty != zonePrettyTwo)
					hudMessage = zonePretty + " & " + zonePrettyTwo + " unlocks in 20 minute(s)";
				else
					hudMessage = zonePretty + " unlocks in 20 minute(s)";
				GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
				break;
			};
			case 900: {
			if(zonePretty != zonePrettyTwo)
					hudMessage = zonePretty + " & " + zonePrettyTwo + " unlocks in 15 minute(s)";
				else
					hudMessage = zonePretty + " unlocks in 15 minute(s)";
				GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
				break;
			};
			case 600: {
				if(zonePretty != zonePrettyTwo)
					hudMessage = zonePretty + " & " + zonePrettyTwo + " unlocks in 10 minute(s)";
				else
					hudMessage = zonePretty + " unlocks in 10 minute(s)";
				GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
				break;
			};
			case 300: {
				if(zonePretty != zonePrettyTwo)
					hudMessage = zonePretty + " & " + zonePrettyTwo + " unlocks in 5 minute(s)";
				else
					hudMessage = zonePretty + " unlocks in 5 minute(s)";
				GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
				break;
			}
			case 60: {
				if(zonePretty != zonePrettyTwo)
					hudMessage = zonePretty + " & " + zonePrettyTwo + " unlocks in 1 minute";
				else
					hudMessage = zonePretty + " unlocks in 1 minute";
				GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
				break;
			}
			case 15: {
				if(zonePretty != zonePrettyTwo)
					hudMessage = zonePretty + " & " + zonePrettyTwo + " unlocks in 15 seconds!";
				else
					hudMessage = zonePretty + " unlocks in 15 seconds!";
				GetGame().GetCallqueue().CallLater(ResetMessage, 6850);
				break;
			}
		}
		UpdateClients();
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateClients()
	{
		CRF_GameModePlayerComponent.GetInstance().UpdateMapMarkers(m_aZonesStatus, m_aZoneObjectNames, bluforSide, opforSide);
	}
};