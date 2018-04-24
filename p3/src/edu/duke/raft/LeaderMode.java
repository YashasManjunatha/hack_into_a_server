package edu.duke.raft;

import java.util.Arrays;
import java.util.Timer;

public class LeaderMode extends RaftMode {


	Timer heartbeatTimer;
	int heartbeatTimeout;
	final int heartbeatTimerID = 3;

	/*
	 Once a candidate wins an election, it
	 becomes leader. It then sends heart beat messages to all of
	 the other servers to establish its authority and prevent new
	 elections.
	 */
	
	/*
	 * If command received from client: append entry to local log,
	 * respond after entry applied to state machine
	 * 
	 * If last log index ≥ nextIndex for a follower: send
	 * AppendEntries RPC with log entries starting at nextIndex
	 * If successful: update nextIndex and matchIndex for follower (§5.3)
	 * If AppendEntries fails because of log inconsistency: decrement nextIndex and retry (§5.3)
	 * If there exists an N such that N > commitIndex, a majority of matchIndex[i] ≥ N, and log[N].term == currentTerm: set commitIndex = N (§5.3, §5.4).
	 * aka entries up to N are all committed
	 */

	public void go () {
		synchronized (mLock) {
			log("switched to leader mode.");	
			
			heartbeat(); // Will call appendEntriesRPC of all servers
			heartbeatTimeout = HEARTBEAT_INTERVAL;
			heartbeatTimer = scheduleTimer(heartbeatTimeout, heartbeatTimerID);

			// To bring a followers log into consistency with its own, the leader
			// must find the latest log entry where the two logs agree, delete any entries in the followers
			// log after that point, and send the follower all of the leaders entries after that point.
			
			// The leader maintains a nextIndex for each follower,
			// which is the index of the next log entry the leader will send to the follower.
			// When a leader first comes into power, it initializes all nextIndex values to the index 
			// just after the last one in its log. If a followers log is inconsistent w/ the leaders, Append Entries
			// will fail. After a rejection, the leader decrements nextIndex and retries. Eventually, they will match.
			
			// replicate the log
			RaftResponses.setTerm(mConfig.getCurrentTerm());
			RaftResponses.clearAppendResponses(mConfig.getCurrentTerm());
			// The goal is to replicate the log prefix on every server.
			int[] nextIndices = new int[mConfig.getNumServers()+1]; // non-zero indexed.
			Arrays.fill(nextIndices, mLog.getLastIndex()+1);
			for (int id = 1; id < mConfig.getNumServers(); id++) {
				if (id == mID) {
					continue;
				}
				
				int nextIndex = nextIndices[id];
				for (int testIndex = nextIndex; testIndex >= 0; testIndex--) {
					// build list of entries beyond this point to use in RPC
					Entry[] entriesBeyondIndex = new Entry[mLog.getLastIndex()-testIndex]; // for now
					
					for( int i = 0; i < entriesBeyondIndex.length; i++ ) {
						entriesBeyondIndex[i] = mLog.getEntry(i+testIndex);
					}
					RaftResponses.clearAppendResponses(mConfig.getCurrentTerm());
					this.remoteAppendEntries(id, mConfig.getCurrentTerm(),mID,testIndex,
							mLog.getEntry(testIndex).term,entriesBeyondIndex, mCommitIndex);
					
					int[] appendResponses = RaftResponses.getAppendResponses(mConfig.getCurrentTerm());
					while (appendResponses.length == 0) {
						// spin
						appendResponses = RaftResponses.getAppendResponses(mConfig.getCurrentTerm());
					}
					
					// when index is common, it should return 0, otherwise that server's current term.
					int returnVal = appendResponses[0];
					
					if (returnVal == 0) {
						// we've found a common index! it's brought into consistency.
						break;
					} else {
						// keep going
						// do something w/ ret val?
					}					
				}
			}
		}
	}
	
	public void heartbeat() {
		RaftResponses.setTerm(mConfig.getCurrentTerm());
		RaftResponses.clearAppendResponses(mConfig.getCurrentTerm());
		for (int id = 1; id <= mConfig.getNumServers(); id++) {
			if (id != mID) {
				log("sending heartbeat to P"+id);
				this.remoteAppendEntries(id, mConfig.getCurrentTerm(),mID,mLog.getLastIndex(),mLog.getLastTerm(),new Entry[0], mCommitIndex);
			}
		}
	}

	// @param candidate’s term
	// @param candidate requesting vote
	// @param index of candidate’s last log entry
	// @param term of candidate’s last log entry
	// @return 0, if server votes for candidate; otherwise, server's
	// current term
	public int requestVote (int candidateTerm,
			int candidateID,
			int lastLogIndex,
			int lastLogTerm) {
		synchronized (mLock) {

			// Determine if requester has larger term, step down if this is the case and
			// vote for them
			// if (notRealLeader)
			//    RaftModeImpl.switchMode((FollowerMode) self);
			//    return 0;
			// return term;

			if (candidateTerm > mConfig.getCurrentTerm()) {
				heartbeatTimer.cancel();
				mConfig.setCurrentTerm(candidateTerm, candidateID);
				RaftServerImpl.setMode(new FollowerMode());
				return 0;
			} else {
				return mConfig.getCurrentTerm();
			}
		}
	}


	// @param leader’s term
	// @param current leader
	// @param index of log entry before entries to append
	// @param term of log entry before entries to append
	// @param entries to append (in order of 0 to append.length-1)
	// @param index of highest committed entry
	// @return 0, if server appended entries; otherwise, server's
	// current term
	public int appendEntries (int leaderTerm,
			int leaderID,
			int prevLogIndex,
			int prevLogTerm,
			Entry[] entries,
			int leaderCommit) {
		synchronized (mLock) {

			if (leaderTerm > mConfig.getCurrentTerm()) {
				heartbeatTimer.cancel();
				RaftServerImpl.setMode(new FollowerMode());
				return 0;
			} else {
				return mConfig.getCurrentTerm();
			}

			// Determine if requester has higher term, step down and vote for them if this is
			// the case
			// if (notRealLeader)
			//    RaftModeImpl.switchMode((FollowerMode) self);
			//    return 0;
			// return term;
		}
		
	}

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == heartbeatTimerID) {
				heartbeat();
				heartbeatTimer.cancel(); // Reset heartbeat timer
				heartbeatTimer = scheduleTimer(heartbeatTimeout,heartbeatTimerID);
			}
		}
	}
	
	public void log(String message) {
		int currentTerm = mConfig.getCurrentTerm();
		System.out.println("S" + mID + "." + currentTerm + " (leader): " + message);
	}
}
