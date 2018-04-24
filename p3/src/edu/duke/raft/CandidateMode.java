package edu.duke.raft;

import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

public class CandidateMode extends RaftMode {
	private Timer electionTimer;
	private Timer voteTimer;
	private final int electionTimerID = 2;
	private final int voteTimerID = 4;
	
	private void setupElectionTimer() {
		int electionTimerTimeout = mConfig.getTimeoutOverride();
		if (electionTimerTimeout == -1) {
			electionTimerTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
		}
		electionTimer = scheduleTimer(electionTimerTimeout, electionTimerID);
	}
	
	private void setupVoteTimer() {
		voteTimer = scheduleTimer(50, voteTimerID);
	}
	

	public void go () {
		synchronized (mLock) {
			int term = mConfig.getCurrentTerm() + 1;
			System.out.println ("S" + 
					mID + 
					"." + 
					term + 
					": switched to candidate mode.");
			electionStart();
		}
	}
	
	private void electionStart() {
		mConfig.setCurrentTerm(mConfig.getCurrentTerm() + 1, mID);
		int term = mConfig.getCurrentTerm();
		
		RaftResponses.setTerm(term);
		RaftResponses.clearVotes(term);
		
		setupElectionTimer();
		setupVoteTimer();

		for (int serverID = 1; serverID <= mConfig.getNumServers(); serverID++) {
			this.remoteRequestVote(serverID, mConfig.getCurrentTerm(), mID, mLog.getLastIndex(), mLog.getLastTerm());
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
			if (candidateID == mID) {
				return 0;
			}
			
			boolean candidateOlderTerm = candidateTerm < mConfig.getCurrentTerm();
			boolean candidateNewerTerm = candidateTerm > mConfig.getCurrentTerm();
			boolean candidateLogUpToDate = lastLogTerm >= mLog.getLastTerm();
			
			if (candidateOlderTerm) {
				return mConfig.getCurrentTerm();
			} else if (candidateNewerTerm) {
				if (candidateLogUpToDate) {
					electionTimer.cancel();
					voteTimer.cancel();
					mConfig.setCurrentTerm(candidateTerm, candidateID);
					RaftServerImpl.setMode(new FollowerMode());
					return mConfig.getCurrentTerm();
				} else {
					electionTimer.cancel();
					voteTimer.cancel();
                    mConfig.setCurrentTerm(candidateTerm, 0);
                    RaftServerImpl.setMode(new FollowerMode());
                    return mConfig.getCurrentTerm();
                }
			} else {//Candidate Same Term
				if (candidateLogUpToDate) {
					electionTimer.cancel();
					voteTimer.cancel();
	                RaftServerImpl.setMode(new FollowerMode());
	                return mConfig.getCurrentTerm();
	            }
	            else {
	            	electionTimer.cancel();
					voteTimer.cancel();
	                mConfig.setCurrentTerm(candidateTerm, 0);
	                electionStart();
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
			if (leaderTerm >= mConfig.getCurrentTerm()) {
				electionTimer.cancel();
				voteTimer.cancel();
				mConfig.setCurrentTerm(leaderTerm, leaderID);
				RaftServerImpl.setMode(new FollowerMode());
				return -1;
			} else {
				return mConfig.getCurrentTerm();
			}
		}
	}

	// @param id of the timer that timed out
	public void handleTimeout (int timerID) {
		synchronized (mLock) {
			if (timerID == voteTimerID) {
				int[] responseVotes = RaftResponses.getVotes(mConfig.getCurrentTerm());
				double voteCount = 0;
				for (int i = 1; i < responseVotes.length; i++) {
					int v = responseVotes[i];
					
					if (v == 0) {
						voteCount++;
					}
					
					if (v > mConfig.getCurrentTerm()) {
                        electionTimer.cancel();
                        voteTimer.cancel();
                        mConfig.setCurrentTerm(v, i);
                        RaftServerImpl.setMode(new FollowerMode());
                        return;
                    }
				}
				
				if (voteCount > ((double)mConfig.getNumServers())/2.0) {
					electionTimer.cancel();
                    voteTimer.cancel();
                    RaftServerImpl.setMode(new LeaderMode());
                    return;
				} else {
					RaftResponses.clearVotes(mConfig.getCurrentTerm());
					
					for (int serverID = 1; serverID <= mConfig.getNumServers(); serverID++) {
						this.remoteRequestVote(serverID, mConfig.getCurrentTerm(), mID, mLog.getLastIndex(), mLog.getLastTerm());
					}
					voteTimer.cancel();
					setupVoteTimer();
				}
			}
			
			if (timerID == electionTimerID) {
				voteTimer.cancel();
				electionTimer.cancel();
				electionStart();
			}
		}
	}
	
//	private void log(String message) {
//		int currentTerm = mConfig.getCurrentTerm();
//		System.out.println("S" + mID + "." + currentTerm + " (Candidate Mode): " + message);
//	}
}
