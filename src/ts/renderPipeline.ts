import { drawCantus } from "./scoreCantus.js";
import { drawCtp } from "./scoreCtp.js";
import { drawCompound } from "./scoreCompound.js";

// Full regeneration: new cantus firmus -> new counterpoint -> new unfolded
// compound melody.
export async function regenerateCantus() {
    await drawCantus();
    await drawCtp();
    await drawCompound();
}

// Counterpoint-only regeneration against the existing cantus firmus.
export async function regenerateCtp() {
    await drawCtp();
    await drawCompound();
}
