package edu.duke.raft;

import java.util.Timer;

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


public class LeaderMode extends RaftMode {
	private Timer heartbeatTimer;
	private int heartbeatTimeout = HEARTBEAT_INTERVAL;
	private final int heartbeatTimerID = 3;
	private int[] nextIndices = new int[mConfig.getNumServers() + 1];
	
	private void setupHeartbeatTimer() {
		heartbeatTimer = scheduleTimer(heartbeatTimeout, heartbeatTimerID);
	}
	
	public void go () {
		synchronized (mLock) {
			int term = mConfig.getCurrentTerm();
			System.out.println ("S" + 
					mID + 
					"." + 
					term + 
					": switched to leader mode.");
			
			setupHeartbeatTimer();
			
			RaftResponses.setTerm(mConfig.getCurrentTerm());
			RaftResponses.clearAppendResponses(mConfig.getCurrentTerm());
			
			for (int i = 1; i <= mConfig.getNumServers(); i++) {
				nextIndices[i] = mLog.getLastIndex() + 1;
			}
			
			for (int id = 1; id <= mConfig.getNumServers(); id++) {
				Entry[] entries;
				int nextIndex = nextIndices[id];
				if (mLog.getEntry(nextIndex) == null) {
					entries = new Entry[0];
				} else {
					entries = new Entry[mLog.getLastIndex() - nextIndex + 1];

					for (int j = nextIndex; j <= mLog.getLastIndex(); j++) {
						entries[j - nextIndex] = mLog.getEntry(j);
					}
				}
				
				this.remoteAppendEntries(id, mConfig.getCurrentTerm(), mID,nextIndex-1, mLog.getEntry(nextIndex-1).term, entries, mCommitIndex);
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
			boolean candidateOlderTerm = candidateTerm < mConfig.getCurrentTerm();
			boolean candidateNewerTerm = candidateTerm > mConfig.getCurrentTerm();
			boolean candidateLogUpToDate = lastLogTerm >= mLog.getLastTerm();
			boolean haventVoted = (mConfig.getVotedFor() == 0 || mConfig.getVotedFor() == candidateID);
			
			if (candidateOlderTerm) {
				return mConfig.getCurrentTerm();
			} else if (candidateNewerTerm) {
				if (candidateLogUpToDate) {
					heartbeatTimer.cancel();
					mConfig.setCurrentTerm(candidateTerm, candidateID);
					RaftServerImpl.setMode(new FollowerMode());
					return 0;
				}
				else {
					heartbeatTimer.cancel();
					mConfig.setCurrentTerm(candidateTerm, 0);
					RaftServerImpl.setMode(new FollowerMode());
					return mConfig.getCurrentTerm();
				}
			} else {//Candidate Same Term
				if (candidateLogUpToDate) {
					if (haventVoted) {
						heartbeatTimer.cancel();
						RaftServerImpl.setMode(new FollowerMode());
						return 0;
					}
					else {
						heartbeatTimer.cancel();
						RaftServerImpl.setMode(new FollowerMode());
						return mConfig.getCurrentTerm();
					}
				} else {
					return mConfig.getCurrentTerm();
				}
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
			if (leaderTerm < mConfig.getCurrentTerm()) {
				return mConfig.getCurrentTerm();
			}
			
			if (leaderTerm > mConfig.getCurrentTerm()) {
				heartbeatTimer.cancel();
				mConfig.setCurrentTerm(leaderTerm, 0);
				RaftServerImpl.setMode(new FollowerMode());
				//return mConfig.getCurrentTerm();
				return -1;
			}
			
			if (leaderID == mID) {
				//return mConfig.getCurrentTerm();
				return -1;
			} else {
				if (entries.length == 0) {
					if (prevLogTerm >= mLog.getLastTerm()) {
						heartbeatTimer.cancel();
						RaftServerImpl.setMode(new FollowerMode());
						//return mConfig.getCurrentTerm();
						return -1;
					} else {
						mConfig.setCurrentTerm(mConfig.getCurrentTerm()+1, mID);
						return mConfig.getCurrentTerm();
					}
				} else {
					if (prevLogTerm > mLog.getLastTerm()) {
						heartbeatTimer.cancel();
						RaftServerImpl.setMode(new FollowerMode());
						//return mConfig.getCurrentTerm();
						return -1;
					} else {
						return 0;
					}
				}
			}
//			int term = mConfig.getCurrentTerm ();
//			int result = term;
//			return result;
		}
	}

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == heartbeatTimerID) {
				heartbeatTimer.cancel();
				setupHeartbeatTimer();
				int[] responses = RaftResponses.getAppendResponses(mConfig.getCurrentTerm());
				
				for (int i = 1; i < responses.length; i++) {
					if (responses[i] > 0) {
						if (responses[i] > mConfig.getCurrentTerm()) {
							heartbeatTimer.cancel();
							mConfig.setCurrentTerm(responses[i], 0);
							RaftServerImpl.setMode(new FollowerMode());
							return;
						}
					}
					if (responses[i] == 0) {
						nextIndices[i] = mLog.getLastTerm() + 1;
					}
					if (responses[i] == -2) {
						nextIndices[i] --;
					}
				}
				RaftResponses.clearAppendResponses(mConfig.getCurrentTerm());
				for (int id = 1; id <= mConfig.getNumServers(); id++) {
					Entry[] entries;
					if (mLog.getEntry(nextIndices[id]) == null) {
						entries = new Entry[0];
					} else {
						entries = new Entry[mLog.getLastIndex() - nextIndices[id] + 1];

						for (int j = nextIndices[id]; j <= mLog.getLastIndex(); j++) {
							entries[j - nextIndices[id]] = mLog.getEntry(j);
						}
					}
					this.remoteAppendEntries(id, mConfig.getCurrentTerm(), mID,nextIndices[id]-1, mLog.getEntry(nextIndices[id]-1).term, entries, mCommitIndex);
				}
			}
		}
	}
	
//	public void log(String message) {
//		int currentTerm = mConfig.getCurrentTerm();
//		System.out.println("S" + mID + "." + currentTerm + " (leader): " + message);
//	}
}
