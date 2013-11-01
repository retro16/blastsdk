E1	EQU	$3F
e2	SET	$2F

aDdRa	FUNC	(E1 | (E2 << 8) | (.1 << 16))
TARGETA	FUNC	24		; Send to a0
	MOVE.L	aDdRa($10), TARGETA()

TARGETA	FUNC	40		; Send to a1
	MOVE.L	ADDRA($20), TARGETA()

	END
