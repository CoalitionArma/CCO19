[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_LinearAASGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_LinearAASGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("US", "auto", "The side designated as blufor", category: "Linear AAS Player Settings")]
	FactionKey bluforSide;
	
	[Attribute("USSR", "auto", "The side designated as opfor", category: "Linear AAS Player Settings")]
	FactionKey opforSide;
	
	[Attribute("", UIWidgets.EditBox, desc: "Array of all zone object names", category: "Linear AAS Zone Settings")]
	ref array<string> m_aZoneObjectNames;
	
	[Attribute("10", "auto", "Min number of players needed to cap a zone", category: "Linear AAS Zone Settings")]
	int m_aMinNumberOfPlayersNeeded;
	
	// - All players withing a zones range
	ref array<SCR_ChimeraCharacter> m_aAllPlayersWithinZoneRange = new array<SCR_ChimeraCharacter>;
	
	// - Each zones status
	[RplProp()]
	ref array<string> m_aZonesStatus = {
		"US:Locked",    // Zone A
		"US:Locked",    // Zone B 
		"N/A:Locked",   // Zone C
		"USSR:Locked",  // Zone D
		"USSR:Locked"   // Zone E
	};
	
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

		//--- Server only
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		GetGame().GetCallqueue().CallLater(UpdateZones, 1000, true);
	}
	
	//------------------------------------------------------------------------------------------------

	// Linear AAS functions

	//------------------------------------------------------------------------------------------------
	
	
	protected void UpdateZones()
	{
		foreach(int i, string zoneName : m_aZoneObjectNames)
		{
			m_aAllPlayersWithinZoneRange.Clear();
			
			IEntity zone = GetGame().GetWorld().FindEntityByName(zoneName);
			
			if(!zone)
				continue;
			
			GetGame().GetWorld().QueryEntitiesBySphere(zone.GetOrigin(), 50, ProcessEntity, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT); // get all entitys withing a 50m radius around the zone
			
			int bluforInZone = 0;
			int opforInZone = 0;
			
			foreach(SCR_ChimeraCharacter player : m_aAllPlayersWithinZoneRange) 
			{
				if(player.GetFactionKey() == bluforSide)
					bluforInZone = bluforInZone + 1;
				
				if(player.GetFactionKey() == opforSide)
					opforInZone = opforInZone + 1;
			};
			
			if (bluforInZone >= m_aMinNumberOfPlayersNeeded && opforInZone < bluforInZone)
				CheckStartCaptureZone(i, bluforSide);
			
			if (opforInZone >= m_aMinNumberOfPlayersNeeded && bluforInZone < opforInZone)
				CheckStartCaptureZone(i, opforSide);
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
	
	protected void CheckStartCaptureZone(int zoneIndex, FactionKey side)
	{
		
	};
};