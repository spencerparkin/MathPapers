

// By "antagonistic," here we mean associations that,
// if clustered, make the presentation of the comparison
// of the graphs much harder to read.  This process/analysis
// isn't perfect, but then again, neither is the comparison.
BOOL EmuGraphComparison::CullAntagonisticAssociations( void )
{
	for( GraphHandleList::Node* pNode = m_graphList.GetHead(); pNode; pNode = m_graphList.GetNext( pNode ) )
	{
		EmuGraphObject* pGraphObject = pNode->value;
		if( !OrderNodesForCrossAnalysis( pGraphObject ) )
			return FALSE;
	}

	while( true )
	{
		PerformAssociationCrossAnalysis();

		Assocation* pMostAntagonisticAssociation = NULL;
		AssociationList::Node* pRemovalNode = NULL;
		for( AssociationList::Node* pNode = m_assocationList.GetHead(); pNode; pNode = m_assocationList.GetNext( pNode ) )
		{
			Association* pAssociation = pNode->value;
			if( pAssociation->m_nCrossCount == 0 )
				continue;

			if( !pMostAntagonisticAssociation || pAssociation->m_nCrossCount > pMostAntagonisticAssociation->m_nCrossCount )
			{
				pMostAntagonisticAssociation = pAssociation;
				pRemovalNode = pNode;
			}
		}

		if( !pMostAntagonisticAssociation )
			break;
		else
		{
			delete pMostAntagonisticAssociation;
			m_associationList.RemoveAt( pRemovalNode );
		}
	}

	return TRUE;
}

void EmuGraphComparison::PerformAssociationCrossAnalysis( void )
{
	for( AssociationList::Node* pNode = m_assocationList.GetHead(); pNode; pNode = m_assocationList.GetNext( pNode ) )
	{
		Association* pAssociation = pNode->value;
		pAssociation->m_nCrossCount = 0;
	}

	for( AssociationList::Node* pNode0 = m_assocationList.GetHead(); pNode0; pNode0 = m_assocationList.GetNext( pNode0 ) )
	{
		const Association* pAssociation0 = pNode0->value;

		for( AssociationList::Node* pNode1 = m_associationList.GetNext( pNode0 ); pNode1; pNode1 = m_associationList.GetNext( pNode1 ) )
		{
			const Association* pAssociation1 = pNode1->value;

			if( AssociationsCross( pAssociation0, pAssociation1 ) )
			{
				pAssociation0->m_nCrossCount++;		// mutable
				pAssociation1->m_nCrossCount++;		// mutable
			}
		}
	}
}

BOOL EmuGraphComparison::AssociationsCross( const Association* pAssociation0, const Association* pAssociation1 )
{
	typedef Genie::Array< int > OrderDeltaSignList;
	OrderDeltaSignList orderDeltaSignList;

	for( NodeHandleList::Node* pNode0 = pAssociation0->m_nodeHandleList.GetHead(); pNode0; pNode0 = pAssociation0->m_nodeHandleList.GetNext( pNode0 ) )
	{
		int hGraphNode0 = pNode0->value;
		EmuGraphNode* pGraphNode0 = EmuGraphNode::DerefHandle( hGraphNode0 );
		if( !pGraphNode0 )
			continue;

		for( NodeHandleList::Node* pNode1 = pAssociation1->m_nodeHandleList.GetHead(); pNode1; pNode1 = pAssociation1->m_nodeHandleList.GetNext( pNode1 ) )
		{
			int hGraphNode1 = pNode1->value;
			EmuGraphNode* pGraphNode1 = EmuGraphNode::DerefHandle( hGraphNode1 );
			if( !pGraphNode1 )
				continue;

			if( pGraphNode0->GetParent() == pGraphNode1->GetParent() )
			{
				int nOrder0 = pGraphNode0->m_nSatelliteData;
				int nOrder1 = pGraphNode1->m_nSatelliteData;

				int nOrderDeltaSign = 0;
				if( nOrder0 < nOrder1 )
					nOrderDeltaSign = +1;
				else if( nOrder0 > nOrder1 )
					nOrderDeltaSign = -1;

				orderDeltaSignList.AddTail( nOrderDeltaSign );

				// There shouldn't exist more than one commonality between any two associations.
				break;
			}
		}
	}

	int nFirstEncounteredNonZeroOrderDeltaSign = 0;
	for( int i = 0; i < orderDeltaSignList.GetCount(); i++ )
	{
		int nOrderDeltaSign = orderDeltaSignList.GetAt(i);
		if( nOrderDeltaSign != 0 )
		{
			if( !nFirstEncounteredNonZeroOrderDeltaSign )
				nFirstEncounteredNonZeroOrderDeltaSign = nOrderDeltaSign;
			else if( nOrderDeltaSign != nFirstEncounteredNonZeroOrderDeltaSign )
				return TRUE;
		}
	}

	return FALSE;
}

// By doing a breadth-first-search here along output execution sites, our orders here are
// a shortest distance to the execute node along input execution sites.
BOOL EmuGraphComparison::OrderNodesForCrossAnalysis( EmuGraphObject* pGraphObject )
{
	EmuGraphObject::NodeList nodeList;
	pGraphObject->PrepareForGraphTraversal( nodeList );

	EmuGraphObject::NodeList executeList;
	pGraphObject->FindAllNodesOfType( "Execute", executeList );
	if( executeList.GetCount() != 1 )
		return FALSE;

	EmuGraphNode* pExecuteNode = executeList.GetHead()->value;
	pExecuteNode->m_nUserSatelliteData = 0;

	EmuGraphObject::NodeList nodeQueue;
	nodeQueue.AddTail( pGraphNode );

	while( nodeQueue.GetCount() > 0 )
	{
		EmuGraphObject::NodeList::Node* pNode = nodeQueue.GetHead();
		EmuGraphObject* pGraphNode = pNode->value;
		nodeQueue.RemoveAt( pNode );

		pGraphNode->m_eTraversalState = EmuGraphNode::TS_VISITED;

		int nOrder = pGraphNode->m_nUserSatelliteData;

		for( int i = 0; i < pGraphNode->GetNumProperties(); i++ )
		{
			Property* pProperty = pGraphNode->GetPropertyAt(i);
			EmuConnectionProperty* pConnectionProperty = pProperty->IsA< EmuConnectionProperty >();
			if( !pConnectionProperty || !pConnectionProperty->IsOutputExecutionSite() )
				continue;

			EmuConnectionProperty::EdgeList& edgeList = pConnectionProperty->GetEdgeList();
			for( EmuConnectionProperty::EdgeList::Node* pEdgeListNode = edgeList.GetHead(); pEdgeListNode; pEdgeListNode = edgeList.GetNext( pEdgeListNode ) )
			{
				int hGraphEdge = pNode->value;
				EmuGraphEdge* pGraphEdge = EmuGraphEdge::DerefHandle( hGraphEdge );
				if( !pGraphEdge || !pGraphEdge->IsValid() )
					continue;

				EmuConnectionProperty* pFollowedConnection = pConnectionProperty->Follow( pGraphEdge );
				ASSERT( pFollowedConnection );
				if( !pFollowedConnection )
					continue;

				EmuGraphNode* pFollowedGraphNode = pFollowedConnection->GetParent()->IsA< EmuGraphNode >();
				ASSERT( pFollowedGraphNode );
				if( !pFollowedGraphNode )
					continue;

				if( pFollowedGraphNode->m_eTraversalState != EmuGraphNode::TS_UNVISITED )
					continue;

				pFollowedGraphNode->m_nUserSatelliteData = nOrder + 1;

				nodeQueue.AddTail( pFollowedGraphNode );
				pFollowedGraphNode->m_eTraversalState = EmuGraphNode::TS_QUEUED;
			}
		}
	}
}