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
	
	[RplProp(), Attribute("", UIWidgets.EditBox, desc: "Array of zone status's at mission start. Example from CCO: \n\n 'US:Locked', \n 'US:Locked', \n 'N/A:Locked', \n 'USSR:Locked', \n 'USSR:Locked' \n\n Each line above represents an index in the array, index 1 is zone A, index 2 is Zone B, etc. The value is bassically: FactionKey:Locked/Unlocked", category: "Linear AAS Zone Settings")]
	ref array<string> m_aZonesStatus;
	
	[Attribute("10", "auto", "Min number of players needed to cap a zone", category: "Linear AAS Zone Settings")]
	int m_aMinNumberOfPlayersNeeded;
	
	// - All players withing a zones range
	ref array<SCR_ChimeraCharacter> m_aAllPlayersWithinZoneRange = new array<SCR_ChimeraCharacter>;
	
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